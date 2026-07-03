#include "gpu_resource_cache.h"
#include <iostream>

namespace aria {

    gpu_resource_cache::gpu_resource_cache(asset_registry* registry, render_backend* backend)
        : m_registry(registry), m_backend(backend), m_fallback_texture_id(0), m_fallback_shader_id(0) {
    }

    gpu_resource_cache::~gpu_resource_cache() { shutdown(); }

    void gpu_resource_cache::init() {
        m_backend->init();
        if (m_fallback_texture_id == 0) {
            m_fallback_texture_id = m_backend->create_fallback_texture();
        }
        // 🔥 HATA ÇÖZÜMÜ: Fallback (Magenta/Hata) shader'ı üretiliyor
        if (m_fallback_shader_id == 0) {
            m_fallback_shader_id = m_backend->create_fallback_shader();
        }
    }

    gpu_texture_id gpu_resource_cache::get_texture_id(handle<texture_asset> h) {
        if (auto it = m_texture_cache.find(h); it != m_texture_cache.end()) {
            return it->second;
        }

        texture_asset* asset = m_registry->get_texture(h);
        if (!asset || asset->raw_data.pixels.empty()) return m_fallback_texture_id;

        gpu_texture_id id = m_backend->create_texture(asset->raw_data);
        m_texture_cache[h] = id;

        asset->raw_data.clear_raw_data();
        return id;
    }

    submesh_runtime_info gpu_resource_cache::get_submesh_info(handle<model_asset> h, size_t sub_mesh_index) {
        // 1. Önce GPU cache kontrolü
        if (auto it = m_model_cache.find(h); it != m_model_cache.end()) {
            if (sub_mesh_index < it->second.size()) {
                return it->second[sub_mesh_index];
            }

            return submesh_runtime_info{};
        }

        // 2. CPU asset'i al
        model_asset* asset = m_registry->get_model(h);

        if (!asset || asset->raw_data.sub_meshes.empty()) {
            return submesh_runtime_info{};
        }

        // 3. Backend GPU objelerini oluşturuyor.
        // Artık gpu_mesh_id değil submesh_bridge dönüyor.
        std::vector<submesh_bridge> bridges = m_backend->create_model(asset->raw_data);

        std::vector<submesh_runtime_info> runtime_infos;
        runtime_infos.reserve(bridges.size());

        for (size_t i = 0; i < bridges.size(); ++i) {
            submesh_runtime_info info;

            // GPU handle
            info.gpu_id = bridges[i].gpu_data;

            // Backend'in ürettiği bounds'u kullan
            info.local_center = bridges[i].bounds.local_center;
            info.local_radius = bridges[i].bounds.local_radius;

            runtime_infos.push_back(info);

            // Eski mesh yapısına GPU id'lerini de yaz
            if (i < asset->sub_meshes.size()) {
                asset->sub_meshes[i].vao = bridges[i].gpu_data.vao_or_buffer_id;
                asset->sub_meshes[i].vbo = bridges[i].gpu_data.vbo;
                asset->sub_meshes[i].ebo = bridges[i].gpu_data.ebo;
            }
        }

        // Cache'e koy
        m_model_cache[h] = runtime_infos;

        // CPU vertex/index verisini boşalt
        asset->raw_data.clear_raw_data();

        if (sub_mesh_index < runtime_infos.size()) {
            return runtime_infos[sub_mesh_index];
        }

        return submesh_runtime_info{};
    }

    gpu_shader_id gpu_resource_cache::get_shader_id(handle<shader_asset> h) {
        if (auto it = m_shader_cache.find(h); it != m_shader_cache.end()) {
            return it->second;
        }

        // Eğer handle invalid ise fallback döndür
        if (!h.is_valid()) {
            return m_fallback_shader_id;
        }

        shader_asset* asset = m_registry->get_shader(h);
        if (!asset || asset->raw_data.vertex_source.empty()) {
            return m_fallback_shader_id;
        }

        // Kuryeden gelen saf veriyi sürücüye (backend) teslim ediyoruz!
        gpu_shader_id id = m_backend->create_shader(asset->raw_data);

        // Eğer compilation başarısız olursa 0 döner, bu durumda cache'e kaydetme!
        if (id == 0) {
            return m_fallback_shader_id;
        }

        m_shader_cache[h] = id;

        // 🔥 AAA: RAM'deki kaynak kod metinlerini temizle, GPU'ya aktarıldı!
        asset->raw_data.clear_raw_data();
        return id;
    }

    void gpu_resource_cache::evict_texture(handle<texture_asset> h) {
        if (auto it = m_texture_cache.find(h); it != m_texture_cache.end()) {
            m_backend->destroy_texture(it->second);
            m_texture_cache.erase(it);
        }
    }

    void gpu_resource_cache::evict_model(handle<model_asset> h) {
        if (auto it = m_model_cache.find(h); it != m_model_cache.end()) {
            std::vector<gpu_mesh_id> ids_to_destroy;
            ids_to_destroy.reserve(it->second.size());

            for (const auto& info : it->second) {
                ids_to_destroy.push_back(info.gpu_id);
            }

            m_backend->destroy_model(ids_to_destroy);
            m_model_cache.erase(it);
        }
    }

    // 🔥 İSTEDİĞİN EVICT SHADER IMPLEMENTASYONU
    void gpu_resource_cache::evict_shader(handle<shader_asset> h) {
        if (auto it = m_shader_cache.find(h); it != m_shader_cache.end()) {
            m_backend->destroy_shader(it->second);
            m_shader_cache.erase(it);
        }
    }

    void gpu_resource_cache::shutdown() {
        // 1. Önce ortak temizlik fonksiyonunu çağır (Kod tekrarı bitti!)
        clear_gpu_cache();

        // 2. Fallback kaynakları temizle (Bunlar cache'de olmadığı için ayrı durmalı)
        if (m_fallback_texture_id != 0) {
            m_backend->destroy_texture(m_fallback_texture_id);
            m_fallback_texture_id = 0;
        }

        if (m_fallback_shader_id != 0) {
            m_backend->destroy_shader(m_fallback_shader_id);
            m_fallback_shader_id = 0;
        }

        // 3. Backend'i en son kapat
        m_backend->shutdown();
    }


    void gpu_resource_cache::clear_gpu_cache() {
        // 1. Texture'ları yok et
        for (auto const& [handle, id] : m_texture_cache) {
            m_backend->destroy_texture(id);
        }
        m_texture_cache.clear();

        // 2. Modelleri yok et (Wrapper'dan çiğ GPU ID'leri çıkarılarak backend'e iletiliyor)
        for (auto const& [handle, runtime_infos] : m_model_cache) {
            std::vector<gpu_mesh_id> ids_to_destroy;
            ids_to_destroy.reserve(runtime_infos.size());

            for (const auto& info : runtime_infos) {
                ids_to_destroy.push_back(info.gpu_id);
            }
            m_backend->destroy_model(ids_to_destroy);
        }
        m_model_cache.clear();

        // 3. Shader'ları yok et
        for (auto const& [handle, id] : m_shader_cache) {
            m_backend->destroy_shader(id);
        }
        m_shader_cache.clear();
    }

} // namespace aria