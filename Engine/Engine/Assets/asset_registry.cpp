#include "asset_registry.h"
#include "asset_loader.h"
#include "../Importers/mesh_generator.h"
#include <iostream>
#include "asset_database.h"


namespace aria {

    // Helper: Eski Mesh üretecini Aria'nın dahili raw formatına dönüştürür ve Bounds hesaplar
    static raw_mesh_data convert_generator_to_raw_mesh(const generated_mesh_data& generated) {
        raw_mesh_data out;
        out.vertices.reserve(generated.vertices.size());
        out.indices = generated.indices;

        glm::vec3 minB(std::numeric_limits<float>::max());
        glm::vec3 maxB(std::numeric_limits<float>::lowest());

        for (const auto& g_vert : generated.vertices) {
            raw_vertex v;
            v.position = g_vert.position;
            v.normal = g_vert.normal;
            v.tex_coord = g_vert.tex_coords;
            v.tangent = glm::vec3(0.0f);

            out.vertices.push_back(v);
            minB = glm::min(minB, g_vert.position);
            maxB = glm::max(maxB, g_vert.position);
        }

        out.local_center = (minB + maxB) * 0.5f;
        float maxDistSq = 0.0f;
        for (const auto& v : out.vertices) {
            glm::vec3 diff = v.position - out.local_center;
            maxDistSq = glm::max(maxDistSq, glm::dot(diff, diff));
        }
        out.local_radius = glm::sqrt(maxDistSq);
        return out;
    }

    asset_registry::asset_registry(asset_database* database)
        : m_database(database) {
    }

    handle<model_asset> asset_registry::load_model(const asset_guid& guid, const std::string& file_path) {
        // Cache kontrolü
        if (auto it = m_model_map.find(guid); it != m_model_map.end()) {
            return it->second;
        }

        raw_model_data modelData;
        modelData.debug_path = file_path;

        // 🧠 SANAL YOL KONTROLÜ (Virtual File System)
        if (file_path == "engine://builtin/cube") {
            generated_mesh_data raw_cube = mesh_generator::generate_cube();
            modelData.sub_meshes.push_back(convert_generator_to_raw_mesh(raw_cube));
            return add_procedural_model(guid, modelData);
        }
        else if (file_path.rfind("engine://builtin/sphere_", 0) == 0) {
            // Yoldan yarıçap değerini güvenli bir şekilde çekiyoruz
            std::string radius_str = file_path.substr(24); // "engine://builtin/sphere_" uzunluğu
            float radius = std::stof(radius_str);

            generated_mesh_data raw_sphere = mesh_generator::generate_sphere(radius, 36, 18);
            modelData.sub_meshes.push_back(convert_generator_to_raw_mesh(raw_sphere));
            return add_procedural_model(guid, modelData);
        }

        // --- GERÇEK DOSYA YÜKLEME SÜRECİ ---
        auto loaded = asset_loader::load_model_to_cpu(file_path);
        if (!loaded) return handle<model_asset>{};

        return add_procedural_model(guid, *loaded);
    }

    handle<model_asset> asset_registry::add_procedural_model(const asset_guid& guid, const raw_model_data& raw_data) {
        uint32_t slot_idx;
        uint32_t gen = 1;

        if (!m_model_free_slots.empty()) {
            slot_idx = m_model_free_slots.back();
            m_model_free_slots.pop_back();
            gen = m_model_slots[slot_idx].generation;
        }
        else {
            m_model_slots.emplace_back();
            slot_idx = static_cast<uint32_t>(m_model_slots.size() - 1);
        }

        auto& slot = m_model_slots[slot_idx];
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.raw_data = raw_data;

        // Mesh metadata paketini (sub_meshes dizisini) kuryeden ayıklayıp dolduruyoruz
        slot.asset.sub_meshes.resize(raw_data.sub_meshes.size());
        for (size_t i = 0; i < raw_data.sub_meshes.size(); ++i) {
            slot.asset.sub_meshes[i].index_count = static_cast<unsigned int>(raw_data.sub_meshes[i].indices.size());
            slot.asset.sub_meshes[i].local_center = raw_data.sub_meshes[i].local_center;
            slot.asset.sub_meshes[i].local_radius = raw_data.sub_meshes[i].local_radius;
        }

        handle<model_asset> h{ slot_idx, gen };
        m_model_map[guid] = h;
        return h;
    }

    model_asset* asset_registry::get_model(handle<model_asset> h) {
        if (h.index < m_model_slots.size() && m_model_slots[h.index].generation == h.generation && m_model_slots[h.index].is_allocated) {
            return &m_model_slots[h.index].asset;
        }
        return nullptr;
    }

    void asset_registry::destroy_model(handle<model_asset> h) {
        if (h.index < m_model_slots.size() && m_model_slots[h.index].generation == h.generation && m_model_slots[h.index].is_allocated) {
            m_model_map.erase(m_model_slots[h.index].asset.guid);
            m_model_slots[h.index].asset.raw_data.clear_raw_data();
            m_model_slots[h.index].asset.sub_meshes.clear();
            m_model_slots[h.index].is_allocated = false;
            m_model_slots[h.index].generation++;
            m_model_free_slots.push_back(h.index);
        }
    }

    // --- DOKU, MATERYAL VE SHADER FONKSİYONLARI (MEVCUT HALİYLE KORUNDU) ---
    handle<texture_asset> asset_registry::load_texture(const asset_guid& guid) {
        // 1) Cache kontrolü
        if (auto it = m_texture_map.find(guid); it != m_texture_map.end())
            return it->second;

        if (guid.is_nil()) return {};

        // 2) 🚀 ARTIK DOĞRUDAN KENDİ DATABASE'İMİZDEN ÇÖZÜYORUZ!
        if (!m_database) {
            std::cout << "[ARIA Registry] Error: Asset database is null!\n";
            return {};
        }
        std::string path = m_database->resolve_path(guid);

        if (path.empty()) {
            std::cout << "[ARIA Registry] Warning: Texture GUID not found in database.\n";
            return {};
        }

        // 3) Diskten CPU'ya yükle
        auto loaded = asset_loader::load_texture_to_cpu(path);
        if (!loaded) return {};

        // 4) Slot ayırma ve doldurma mantığı (Senin orijinal kodun)
        uint32_t idx = m_texture_free_slots.empty() ? (m_texture_slots.emplace_back(), m_texture_slots.size() - 1) : (m_texture_free_slots.back());
        if (!m_texture_free_slots.empty()) m_texture_free_slots.pop_back();

        auto& slot = m_texture_slots[idx];
        slot.is_allocated = true; slot.asset.guid = guid; slot.asset.raw_data = *loaded;
        slot.asset.width = loaded->width; slot.asset.height = loaded->height; slot.asset.channels = loaded->channels;
        return m_texture_map[guid] = handle<texture_asset>{ idx, slot.generation };
    }

    texture_asset* asset_registry::get_texture(handle<texture_asset> h) {
        return (h.index < m_texture_slots.size() && m_texture_slots[h.index].generation == h.generation && m_texture_slots[h.index].is_allocated) ? &m_texture_slots[h.index].asset : nullptr;
    }

    void asset_registry::destroy_texture(handle<texture_asset> h) {
        if (h.index < m_texture_slots.size() && m_texture_slots[h.index].generation == h.generation && m_texture_slots[h.index].is_allocated) {
            m_texture_map.erase(m_texture_slots[h.index].asset.guid);
            m_texture_slots[h.index].asset.raw_data.clear_raw_data();
            m_texture_slots[h.index].is_allocated = false; m_texture_slots[h.index].generation++;
            m_texture_free_slots.push_back(h.index);
        }
    }

    handle<material_asset> asset_registry::load_material(const asset_guid& guid, const std::string& file_path)
    {
         if (auto it = m_material_map.find(guid); it != m_material_map.end())
             return it->second;

        // 2. Dosyayı CPU'ya yükle
        auto loaded = asset_loader::load_material_to_cpu(file_path);
        if (!loaded)
            return {};

        // 3. Slot bulma/açma mantığı
        uint32_t idx;
        if (m_material_free_slots.empty())
        {
            m_material_slots.emplace_back();
            idx = static_cast<uint32_t>(m_material_slots.size() - 1);
        }
        else
        {
            idx = m_material_free_slots.back();
            m_material_free_slots.pop_back();
        }

        auto& slot = m_material_slots[idx];

        // Verileri kopyala
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.name = loaded->name;

		slot.asset.mode = loaded->mode;
		slot.asset.alpha_threshold = loaded->alpha_threshold;

        slot.asset.base_color = loaded->base_color;
        slot.asset.specular_strength = loaded->specular_strength;
        slot.asset.shininess = loaded->shininess;
        slot.asset.normal_strength = loaded->normal_strength;
        slot.asset.texture_scale = loaded->texture_scale;

       

        // A. Diffuse Texture Bağlantısı
        if (!loaded->diffuse_texture_guid.is_nil())
        {
            // load_texture artık tek parametre (GUID) alıyor, mis gibi!
            slot.asset.diffuse_texture_handle = this->load_texture(loaded->diffuse_texture_guid);
        }
        else
        {
            slot.asset.diffuse_texture_handle = {};
        }

        // B. Normal Texture Bağlantısı
        if (!loaded->normal_texture_guid.is_nil())
        {
            slot.asset.normal_texture_handle = this->load_texture(loaded->normal_texture_guid);
        }
        else
        {
            slot.asset.normal_texture_handle = {};
        }

        // C. Shader Bağlantısı
        if (!loaded->shader_guid.is_nil())
        {
            // load_shader fonksiyonunu da aynı mantıkta tek parametreli (GUID) yaparsın
            slot.asset.shader_handle = this->load_shader(loaded->shader_guid);
           
        }
        else
        {
            slot.asset.shader_handle = {};
        }
        // ========================================================================

        handle<material_asset> h{ idx, slot.generation };
        m_material_map[guid] = h;

        return h;
    }
    
    material_asset* asset_registry::get_material(handle<material_asset> h)
    {
        if (h.index >= m_material_slots.size()) return nullptr;

        auto& slot = m_material_slots[h.index];

        if (!slot.is_allocated || slot.generation != h.generation)
            return nullptr;

        if (slot.asset.guid.is_nil())
            return nullptr;

        return &slot.asset;
    }

    void asset_registry::destroy_material(handle<material_asset> h)
    {
        if (h.index >= m_material_slots.size())
            return;

        auto& slot = m_material_slots[h.index];

        if (!slot.is_allocated || slot.generation != h.generation)
            return;

        m_material_map.erase(slot.asset.guid);

        slot.asset = material_asset{};

        slot.is_allocated = false;
        slot.generation++;

        std::cout << "[DESTROY MAT] index=" << h.index << " gen=" << h.generation << "\n";

        m_material_free_slots.push_back(h.index);
    }

    handle<shader_asset> asset_registry::load_shader(const asset_guid& guid) {
        // 1) Cache kontrolü
        if (auto it = m_shader_map.find(guid); it != m_shader_map.end()) {
            std::cout << "[SHADER DEBUG] Cache HIT, guid=" << uuids::to_string(guid) << "\n";
            return it->second;
        }

        if (guid.is_nil()) {
            std::cout << "[SHADER DEBUG] GUID is nil, returning empty handle\n";
            return {};
        }

        // 2) 🚀 DOĞRUDAN KENDİ DATABASE'İMİZDEN ÇÖZÜYORUZ!
        if (!m_database) {
            std::cout << "[SHADER DEBUG] Database is null!\n";
            return {};
        }

        std::string path = m_database->resolve_path(guid);

        if (path.empty()) {
            std::cout << "[SHADER DEBUG] Path is empty, shader not found in database\n";
            return {};
        }

        // 3) Diskten CPU'ya yükle
        auto loaded = asset_loader::load_shader_to_cpu(path);
        if (!loaded) {
            std::cout << "[SHADER DEBUG] load_shader_to_cpu FAILED, path=" << path << "\n";
            return {};
        }

        std::cout << "[SHADER DEBUG] Successfully loaded from disk, path=" << path << "\n";

        // 4) Slot ayırma ve doldurma
        uint32_t idx;
        uint32_t generation;

        if (m_shader_free_slots.empty()) {
            m_shader_slots.emplace_back();
            idx = static_cast<uint32_t>(m_shader_slots.size() - 1);
            generation = m_shader_slots[idx].generation;
        } else {
            idx = m_shader_free_slots.back();
            m_shader_free_slots.pop_back();
            generation = m_shader_slots[idx].generation;
        }

        auto& slot = m_shader_slots[idx];
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.raw_data = *loaded;


        handle<shader_asset> h{ idx, generation };
        m_shader_map[guid] = h;
        return h;
    }

    shader_asset* asset_registry::get_shader(handle<shader_asset> h) {
        return (h.index < m_shader_slots.size() && m_shader_slots[h.index].generation == h.generation && m_shader_slots[h.index].is_allocated) ? &m_shader_slots[h.index].asset : nullptr;
    }

    void asset_registry::destroy_shader(handle<shader_asset> h) {
        if (h.index < m_shader_slots.size() && m_shader_slots[h.index].generation == h.generation && m_shader_slots[h.index].is_allocated) {
            m_shader_map.erase(m_shader_slots[h.index].asset.guid);
            m_shader_slots[h.index].asset.raw_data.clear_raw_data();
            m_shader_slots[h.index].is_allocated = false; m_shader_slots[h.index].generation++;
            m_shader_free_slots.push_back(h.index);
        }
    }

    void asset_registry::clear_registry() {
        m_texture_slots.clear(); m_texture_free_slots.clear(); m_texture_map.clear();
        m_model_slots.clear(); m_model_free_slots.clear(); m_model_map.clear();
        m_material_slots.clear(); m_material_free_slots.clear(); m_material_map.clear();
        m_shader_slots.clear(); m_shader_free_slots.clear(); m_shader_map.clear();
    }

} // namespace aria