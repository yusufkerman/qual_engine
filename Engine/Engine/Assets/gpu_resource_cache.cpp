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

    gpu_mesh_id gpu_resource_cache::get_mesh_gpu_id(handle<model_asset> h, size_t sub_mesh_index) {
        if (auto it = m_model_cache.find(h); it != m_model_cache.end()) {
            if (sub_mesh_index < it->second.size()) {
                std::cout
                    << "[GPU CACHE] MESH CACHE HIT "
                    << "handle=(" << h.index << "," << h.generation << ") "
                    << "submesh=" << sub_mesh_index
                    << " vao=" << it->second[sub_mesh_index].vao_or_buffer_id
                    << std::endl;
                return it->second[sub_mesh_index];
            }
            else {
                std::cout
                    << "[GPU CACHE] ERROR: submesh index " << sub_mesh_index 
                    << " out of range (count=" << it->second.size() << ")\n";
                return gpu_mesh_id{};
            }
        }

        model_asset* asset = m_registry->get_model(h);
        if (!asset || asset->raw_data.sub_meshes.empty()) {
            std::cout << "[GPU CACHE] ERROR: Model asset not found or empty\n";
            return gpu_mesh_id{};
        }

        std::vector<gpu_mesh_id> ids = m_backend->create_model(asset->raw_data);
        m_model_cache[h] = ids;

        std::cout
            << "[GPU CACHE] MESH CACHE MISS - Created " << ids.size() << " meshes for handle("
            << h.index << "," << h.generation << ")\n";

        for (size_t i = 0; i < ids.size(); ++i) {
            asset->sub_meshes[i].vao = ids[i].vao_or_buffer_id;
            asset->sub_meshes[i].vbo = ids[i].vbo;
            asset->sub_meshes[i].ebo = ids[i].ebo;
        }

        asset->raw_data.clear_raw_data();

        if (sub_mesh_index < ids.size()) {
            return ids[sub_mesh_index];
        }
        else {
            std::cout << "[GPU CACHE] ERROR: submesh index " << sub_mesh_index 
                << " out of range (count=" << ids.size() << ")\n";
            return gpu_mesh_id{};
        }
    }

    gpu_shader_id gpu_resource_cache::get_shader_id(handle<shader_asset> h) {
        if (auto it = m_shader_cache.find(h); it != m_shader_cache.end()) {
            std::cout << "[GPU CACHE] Shader cache HIT for handle(" << h.index << "," << h.generation << ") -> GPU ID: " << it->second << "\n";
            return it->second;
        }

        // Eğer handle invalid ise fallback döndür
        if (!h.is_valid()) {
            std::cout << "[GPU CACHE] Invalid shader handle, returning fallback shader ID: " << m_fallback_shader_id << "\n";
            return m_fallback_shader_id;
        }

        shader_asset* asset = m_registry->get_shader(h);
        if (!asset || asset->raw_data.vertex_source.empty()) {
            std::cout << "[GPU CACHE] Shader asset not found or empty, returning fallback shader ID: " << m_fallback_shader_id << "\n";
            return m_fallback_shader_id;
        }

        // Kuryeden gelen saf veriyi sürücüye (backend) teslim ediyoruz!
        gpu_shader_id id = m_backend->create_shader(asset->raw_data);

        // Eğer compilation başarısız olursa 0 döner, bu durumda cache'e kaydetme!
        if (id == 0) {
            std::cout << "[GPU CACHE] Shader GPU compilation FAILED for handle(" << h.index << "," << h.generation << "), returning fallback\n";
            return m_fallback_shader_id;
        }

        std::cout << "[GPU CACHE] Created shader GPU ID: " << id << " from asset handle(" << h.index << "," << h.generation << ")\n";
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
            m_backend->destroy_model(it->second);
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


    void aria::gpu_resource_cache::clear_gpu_cache() {
        // 1. Texture'ları yok et
        for (auto const& [handle, id] : m_texture_cache) {
            m_backend->destroy_texture(id);
        }
        m_texture_cache.clear();

        // 2. Modelleri yok et
        for (auto const& [handle, mesh_ids] : m_model_cache) {
            m_backend->destroy_model(mesh_ids);
        }
        m_model_cache.clear();

        // 3. Shader'ları yok et
        for (auto const& [handle, id] : m_shader_cache) {
            m_backend->destroy_shader(id);
        }
        m_shader_cache.clear();
    }

} // namespace aria