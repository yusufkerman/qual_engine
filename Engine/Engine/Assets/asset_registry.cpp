/**
 * @file asset_registry.cpp
 * @brief Varlıkların CPU üzerindeki tahsis/serbest bırakma işlemlerini ve nesilsel (generational) kayıt defterini yönetir.
 */

#include "asset_registry.h"
#include "asset_loader.h"
#include "../Importers/mesh_generator.h"
#include <iostream>
#include "asset_database.h"

namespace aria {

    /**
     * @brief Prosedürel olarak üretilmiş eski tip mesh verisini, Aria'nın dahili ham (raw) formatına dönüştürür ve sınırlarını hesaplar.
     *
     * @param generated Üretilmiş olan kaynak mesh verisi.
     * @return raw_mesh_data Dönüştürülmüş ve yerel sınırları hesaplanmış ham mesh verisi.
     */
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
        // Önbellek (Cache) kontrolü
        if (auto it = m_model_map.find(guid); it != m_model_map.end()) {
            return it->second;
        }

        raw_model_data modelData;
        modelData.debug_path = file_path;

        // Sanal Yol (Virtual File System) Kontrolü
        if (file_path == "engine://builtin/cube") {
            generated_mesh_data raw_cube = mesh_generator::generate_cube();
            modelData.sub_meshes.push_back(convert_generator_to_raw_mesh(raw_cube));
            return add_procedural_model(guid, modelData);
        }
        else if (file_path.rfind("engine://builtin/sphere_", 0) == 0) {
            // Yoldan yarıçap değerini güvenli bir şekilde ayrıştır
            std::string radius_str = file_path.substr(24); // "engine://builtin/sphere_" uzunluğu
            float radius = std::stof(radius_str);

            generated_mesh_data raw_sphere = mesh_generator::generate_sphere(radius, 36, 18);
            modelData.sub_meshes.push_back(convert_generator_to_raw_mesh(raw_sphere));
            return add_procedural_model(guid, modelData);
        }

        // Gerçek Dosya Yükleme Süreci
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

        // Mesh meta veri paketini (sub_meshes dizisini) kuryeden ayıklayıp dolduruyoruz
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

    // ========================================================================
    // DOKU (TEXTURE) YÖNETİMİ
    // ========================================================================

    handle<texture_asset> asset_registry::load_texture(const asset_guid& guid) {
        if (auto it = m_texture_map.find(guid); it != m_texture_map.end())
            return it->second;

        if (guid.is_nil()) return {};

        if (!m_database) {
            std::cout << "[ARIA Registry] Hata: Varlik veritabani (asset_database) tanimsiz!\n";
            return {};
        }

        std::string path = m_database->resolve_path(guid);

        if (path.empty()) {
            std::cout << "[ARIA Registry] Uyari: Doku GUID'si veritabaninda bulunamadi.\n";
            return {};
        }

        auto loaded = asset_loader::load_texture_to_cpu(path);
        if (!loaded) return {};

        uint32_t idx = m_texture_free_slots.empty()
            ? (m_texture_slots.emplace_back(), static_cast<uint32_t>(m_texture_slots.size() - 1))
            : (m_texture_free_slots.back());

        if (!m_texture_free_slots.empty()) m_texture_free_slots.pop_back();

        auto& slot = m_texture_slots[idx];
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.raw_data = *loaded;
        slot.asset.width = loaded->width;
        slot.asset.height = loaded->height;
        slot.asset.channels = loaded->channels;

        return m_texture_map[guid] = handle<texture_asset>{ idx, slot.generation };
    }

    texture_asset* asset_registry::get_texture(handle<texture_asset> h) {
        return (h.index < m_texture_slots.size() && m_texture_slots[h.index].generation == h.generation && m_texture_slots[h.index].is_allocated)
            ? &m_texture_slots[h.index].asset
            : nullptr;
    }

    void asset_registry::destroy_texture(handle<texture_asset> h) {
        if (h.index < m_texture_slots.size() && m_texture_slots[h.index].generation == h.generation && m_texture_slots[h.index].is_allocated) {
            m_texture_map.erase(m_texture_slots[h.index].asset.guid);
            m_texture_slots[h.index].asset.raw_data.clear_raw_data();
            m_texture_slots[h.index].is_allocated = false;
            m_texture_slots[h.index].generation++;
            m_texture_free_slots.push_back(h.index);
        }
    }

    // ========================================================================
    // MATERYAL (MATERIAL) YÖNETİMİ
    // ========================================================================

    handle<material_asset> asset_registry::load_material(const asset_guid& guid, const std::string& file_path) {
        if (auto it = m_material_map.find(guid); it != m_material_map.end())
            return it->second;

        auto loaded = asset_loader::load_material_to_cpu(file_path);
        if (!loaded)
            return {};

        uint32_t idx;
        if (m_material_free_slots.empty()) {
            m_material_slots.emplace_back();
            idx = static_cast<uint32_t>(m_material_slots.size() - 1);
        }
        else {
            idx = m_material_free_slots.back();
            m_material_free_slots.pop_back();
        }

        auto& slot = m_material_slots[idx];

        // Temel Verileri Kopyala
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.name = loaded->name;
        slot.asset.mode = loaded->mode;
        slot.asset.alpha_threshold = loaded->alpha_threshold;
        slot.asset.base_color = loaded->base_color;
        slot.asset.metallic = loaded->metallic;
        slot.asset.roughness = loaded->roughness;
        slot.asset.ao = loaded->ao;
        slot.asset.emissive_color = loaded->emissive_color;
        slot.asset.emissive_intensity = loaded->emissive_intensity;

        slot.asset.specular_strength = loaded->specular_strength;
        slot.asset.shininess = loaded->shininess;
        slot.asset.normal_strength = loaded->normal_strength;
        slot.asset.texture_scale = loaded->texture_scale;

        // Bağımlı Varlıkları Yükle (Doku ve Shader)
        slot.asset.diffuse_texture_handle = (!loaded->diffuse_texture_guid.is_nil())
            ? this->load_texture(loaded->diffuse_texture_guid) : handle<texture_asset>{};

        slot.asset.normal_texture_handle = (!loaded->normal_texture_guid.is_nil())
            ? this->load_texture(loaded->normal_texture_guid) : handle<texture_asset>{};

        slot.asset.metallic_texture_handle = (!loaded->metallic_texture_guid.is_nil())
            ? this->load_texture(loaded->metallic_texture_guid) : handle<texture_asset>{};

        slot.asset.roughness_texture_handle = (!loaded->roughness_texture_guid.is_nil())
            ? this->load_texture(loaded->roughness_texture_guid) : handle<texture_asset>{};

        slot.asset.ao_texture_handle = (!loaded->ao_texture_guid.is_nil())
            ? this->load_texture(loaded->ao_texture_guid) : handle<texture_asset>{};

        slot.asset.emissive_texture_handle = (!loaded->emissive_texture_guid.is_nil())
            ? this->load_texture(loaded->emissive_texture_guid) : handle<texture_asset>{};

        slot.asset.specular_texture_handle = (!loaded->specular_texture_guid.is_nil())
            ? this->load_texture(loaded->specular_texture_guid) : handle<texture_asset>{};

        slot.asset.shader_handle = (!loaded->shader_guid.is_nil())
            ? this->load_shader(loaded->shader_guid) : handle<shader_asset>{};

        handle<material_asset> h{ idx, slot.generation };
        m_material_map[guid] = h;

        return h;
    }

    handle<material_asset> asset_registry::reload_material(const asset_guid& guid, const std::string& file_path) {
        auto loaded = asset_loader::load_material_to_cpu(file_path);
        if (!loaded)
            return {};

        uint32_t idx = 0;
        auto it = m_material_map.find(guid);
        if (it != m_material_map.end()) {
            idx = it->second.index;
        }
        else {
            return load_material(guid, file_path);
        }

        if (idx >= m_material_slots.size()) {
            return load_material(guid, file_path);
        }

        auto& slot = m_material_slots[idx];

        // Slot verilerini diskteki yeni verilerle güncelle (Hot-Reload)
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.name = loaded->name;
        slot.asset.mode = loaded->mode;
        slot.asset.alpha_threshold = loaded->alpha_threshold;
        slot.asset.base_color = loaded->base_color;
        slot.asset.metallic = loaded->metallic;
        slot.asset.roughness = loaded->roughness;
        slot.asset.ao = loaded->ao;
        slot.asset.emissive_color = loaded->emissive_color;
        slot.asset.emissive_intensity = loaded->emissive_intensity;

        slot.asset.specular_strength = loaded->specular_strength;
        slot.asset.shininess = loaded->shininess;
        slot.asset.normal_strength = loaded->normal_strength;
        slot.asset.texture_scale = loaded->texture_scale;

        // Bağımlı Varlıkları Yükle (Doku ve Shader)
        slot.asset.diffuse_texture_handle = (!loaded->diffuse_texture_guid.is_nil())
            ? this->load_texture(loaded->diffuse_texture_guid) : handle<texture_asset>{};

        slot.asset.normal_texture_handle = (!loaded->normal_texture_guid.is_nil())
            ? this->load_texture(loaded->normal_texture_guid) : handle<texture_asset>{};

        slot.asset.metallic_texture_handle = (!loaded->metallic_texture_guid.is_nil())
            ? this->load_texture(loaded->metallic_texture_guid) : handle<texture_asset>{};

        slot.asset.roughness_texture_handle = (!loaded->roughness_texture_guid.is_nil())
            ? this->load_texture(loaded->roughness_texture_guid) : handle<texture_asset>{};

        slot.asset.ao_texture_handle = (!loaded->ao_texture_guid.is_nil())
            ? this->load_texture(loaded->ao_texture_guid) : handle<texture_asset>{};

        slot.asset.emissive_texture_handle = (!loaded->emissive_texture_guid.is_nil())
            ? this->load_texture(loaded->emissive_texture_guid) : handle<texture_asset>{};

        slot.asset.specular_texture_handle = (!loaded->specular_texture_guid.is_nil())
            ? this->load_texture(loaded->specular_texture_guid) : handle<texture_asset>{};

        slot.asset.shader_handle = (!loaded->shader_guid.is_nil())
            ? this->load_shader(loaded->shader_guid) : handle<shader_asset>{};

        std::cout << "[Aria Registry] Materyal basariyla yeniden yuklendi (Hot-Reload): " << loaded->name << "\n";
        return it->second;
    }

    material_asset* asset_registry::get_material(handle<material_asset> h) {
        if (h.index >= m_material_slots.size()) return nullptr;

        auto& slot = m_material_slots[h.index];

        if (!slot.is_allocated || slot.generation != h.generation || slot.asset.guid.is_nil())
            return nullptr;

        return &slot.asset;
    }

    void asset_registry::destroy_material(handle<material_asset> h) {
        if (h.index >= m_material_slots.size())
            return;

        auto& slot = m_material_slots[h.index];

        if (!slot.is_allocated || slot.generation != h.generation)
            return;

        m_material_map.erase(slot.asset.guid);
        slot.asset = material_asset{};
        slot.is_allocated = false;
        slot.generation++;

        std::cout << "[ARIA Registry] Materyal serbest birakildi: index=" << h.index << " gen=" << h.generation << "\n";

        m_material_free_slots.push_back(h.index);
    }

    // ========================================================================
    // SHADER YÖNETİMİ
    // ========================================================================

    handle<shader_asset> asset_registry::load_shader(const asset_guid& guid) {
        if (auto it = m_shader_map.find(guid); it != m_shader_map.end()) {
            return it->second;
        }

        if (guid.is_nil()) {
            return {};
        }

        if (!m_database) {
            std::cout << "[ARIA Registry] Hata: Veritabani tanimsiz!\n";
            return {};
        }

        std::string path = m_database->resolve_path(guid);

        if (path.empty()) {
            std::cout << "[ARIA Registry] Uyari: Shader dosya yolu veritabaninda bulunamadi.\n";
            return {};
        }

        auto loaded = asset_loader::load_shader_to_cpu(path);
        if (!loaded) {
            std::cout << "[ARIA Registry] Hata: Shader yuklenemedi (load_shader_to_cpu basarisiz), path=" << path << "\n";
            return {};
        }

        uint32_t idx;
        uint32_t generation;

        if (m_shader_free_slots.empty()) {
            m_shader_slots.emplace_back();
            idx = static_cast<uint32_t>(m_shader_slots.size() - 1);
            generation = m_shader_slots[idx].generation;
        }
        else {
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
        return (h.index < m_shader_slots.size() && m_shader_slots[h.index].generation == h.generation && m_shader_slots[h.index].is_allocated)
            ? &m_shader_slots[h.index].asset
            : nullptr;
    }

    void asset_registry::destroy_shader(handle<shader_asset> h) {
        if (h.index < m_shader_slots.size() && m_shader_slots[h.index].generation == h.generation && m_shader_slots[h.index].is_allocated) {
            m_shader_map.erase(m_shader_slots[h.index].asset.guid);
            m_shader_slots[h.index].asset.raw_data.clear_raw_data();
            m_shader_slots[h.index].is_allocated = false;
            m_shader_slots[h.index].generation++;
            m_shader_free_slots.push_back(h.index);
        }
    }

    // ========================================================================
    // İSKELET (SKELETON) YÖNETİMİ
    // ========================================================================

    static const uuids::uuid asset_internal_namespace = uuids::uuid{ {
        0x6b, 0xa7, 0xb8, 0x10,
        0x9d, 0xad, 0x11, 0xd1,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    } };

    handle<skeleton_asset> asset_registry::load_skeleton(const asset_guid& guid, const std::string& file_path) {
        if (auto it = m_skeleton_map.find(guid); it != m_skeleton_map.end()) {
            return it->second;
        }

        if (guid.is_nil()) return {};

        auto loaded = asset_loader::load_skeleton_to_cpu(file_path);
        if (!loaded) return {};

        return add_procedural_skeleton(guid, *loaded);
    }

    handle<skeleton_asset> asset_registry::add_procedural_skeleton(const asset_guid& guid, const raw_skeleton_data& raw_data) {
        uint32_t slot_idx;
        uint32_t gen = 1;

        if (!m_skeleton_free_slots.empty()) {
            slot_idx = m_skeleton_free_slots.back();
            m_skeleton_free_slots.pop_back();
            gen = m_skeleton_slots[slot_idx].generation;
        }
        else {
            m_skeleton_slots.emplace_back();
            slot_idx = static_cast<uint32_t>(m_skeleton_slots.size() - 1);
        }

        auto& slot = m_skeleton_slots[slot_idx];
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.raw_data = raw_data;

        handle<skeleton_asset> h{ slot_idx, gen };
        m_skeleton_map[guid] = h;
        return h;
    }

    skeleton_asset* asset_registry::get_skeleton(handle<skeleton_asset> h) {
        if (h.index < m_skeleton_slots.size() && m_skeleton_slots[h.index].generation == h.generation && m_skeleton_slots[h.index].is_allocated) {
            return &m_skeleton_slots[h.index].asset;
        }
        return nullptr;
    }

    void asset_registry::destroy_skeleton(handle<skeleton_asset> h) {
        if (h.index < m_skeleton_slots.size() && m_skeleton_slots[h.index].generation == h.generation && m_skeleton_slots[h.index].is_allocated) {
            m_skeleton_map.erase(m_skeleton_slots[h.index].asset.guid);
            m_skeleton_slots[h.index].asset.raw_data.clear_raw_data();
            m_skeleton_slots[h.index].is_allocated = false;
            m_skeleton_slots[h.index].generation++;
            m_skeleton_free_slots.push_back(h.index);
        }
    }

    // ========================================================================
    // ANİMASYON KLİBİ (ANIMATION CLIP) YÖNETİMİ
    // ========================================================================

    handle<animation_clip_asset> asset_registry::load_animation(const asset_guid& guid, const std::string& file_path) {
        if (auto it = m_animation_map.find(guid); it != m_animation_map.end()) {
            return it->second;
        }

        if (guid.is_nil()) return {};

        auto loaded = asset_loader::load_animations_to_cpu(file_path);
        if (!loaded || loaded->empty()) return {};

        return add_procedural_animation(guid, (*loaded)[0]);
    }

    handle<animation_clip_asset> asset_registry::add_procedural_animation(const asset_guid& guid, const raw_animation_data& raw_data) {
        uint32_t slot_idx;
        uint32_t gen = 1;

        if (!m_animation_free_slots.empty()) {
            slot_idx = m_animation_free_slots.back();
            m_animation_free_slots.pop_back();
            gen = m_animation_slots[slot_idx].generation;
        }
        else {
            m_animation_slots.emplace_back();
            slot_idx = static_cast<uint32_t>(m_animation_slots.size() - 1);
        }

        auto& slot = m_animation_slots[slot_idx];
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.raw_data = raw_data;

        handle<animation_clip_asset> h{ slot_idx, gen };
        m_animation_map[guid] = h;
        return h;
    }

    animation_clip_asset* asset_registry::get_animation(handle<animation_clip_asset> h) {
        if (h.index < m_animation_slots.size() && m_animation_slots[h.index].generation == h.generation && m_animation_slots[h.index].is_allocated) {
            return &m_animation_slots[h.index].asset;
        }
        return nullptr;
    }

    void asset_registry::destroy_animation(handle<animation_clip_asset> h) {
        if (h.index < m_animation_slots.size() && m_animation_slots[h.index].generation == h.generation && m_animation_slots[h.index].is_allocated) {
            m_animation_map.erase(m_animation_slots[h.index].asset.guid);
            m_animation_slots[h.index].asset.raw_data.clear_raw_data();
            m_animation_slots[h.index].is_allocated = false;
            m_animation_slots[h.index].generation++;
            m_animation_free_slots.push_back(h.index);
        }
    }

    // ========================================================================
    // İSKELETLİ MODEL (SKINNED MODEL) YÖNETİMİ
    // ========================================================================

    handle<skinned_model_asset> asset_registry::load_skinned_model(const asset_guid& guid, const std::string& file_path) {
        if (auto it = m_skinned_model_map.find(guid); it != m_skinned_model_map.end()) {
            return it->second;
        }

        if (guid.is_nil()) return {};

        auto loaded = asset_loader::load_skinned_model_to_cpu(file_path);
        if (!loaded) return {};

        return add_procedural_skinned_model(guid, *loaded);
    }

    handle<skinned_model_asset> asset_registry::add_procedural_skinned_model(const asset_guid& guid, const raw_skinned_model_data& raw_data) {
        uint32_t slot_idx;
        uint32_t gen = 1;

        if (!m_skinned_model_free_slots.empty()) {
            slot_idx = m_skinned_model_free_slots.back();
            m_skinned_model_free_slots.pop_back();
            gen = m_skinned_model_slots[slot_idx].generation;
        }
        else {
            m_skinned_model_slots.emplace_back();
            slot_idx = static_cast<uint32_t>(m_skinned_model_slots.size() - 1);
        }

        auto& slot = m_skinned_model_slots[slot_idx];
        slot.is_allocated = true;
        slot.asset.guid = guid;
        slot.asset.raw_data = raw_data;

        // Submesh meta verilerini doldur
        slot.asset.sub_meshes.resize(raw_data.sub_meshes.size());
        for (size_t i = 0; i < raw_data.sub_meshes.size(); ++i) {
            slot.asset.sub_meshes[i].index_count = static_cast<unsigned int>(raw_data.sub_meshes[i].indices.size());
            slot.asset.sub_meshes[i].local_center = raw_data.sub_meshes[i].local_center;
            slot.asset.sub_meshes[i].local_radius = raw_data.sub_meshes[i].local_radius;
        }

        // Eğer iskelet varsa, alt iskelet varlığı olarak da kaydet ve bağla
        if (!raw_data.skeleton.bones.empty()) {
            uuids::uuid_name_generator gen_sk(asset_internal_namespace);
            asset_guid sk_guid = gen_sk(raw_data.debug_path + ":skeleton");
            slot.asset.skeleton_handle = add_procedural_skeleton(sk_guid, raw_data.skeleton);
        }

        // Varsa animasyonları da kaydet ve bağla
        for (size_t i = 0; i < raw_data.animations.size(); ++i) {
            uuids::uuid_name_generator gen_anim(asset_internal_namespace);
            asset_guid anim_guid = gen_anim(raw_data.debug_path + ":anim:" + std::to_string(i));
            slot.asset.animation_handles.push_back(add_procedural_animation(anim_guid, raw_data.animations[i]));
        }

        handle<skinned_model_asset> h{ slot_idx, gen };
        m_skinned_model_map[guid] = h;
        return h;
    }

    skinned_model_asset* asset_registry::get_skinned_model(handle<skinned_model_asset> h) {
        if (h.index < m_skinned_model_slots.size() && m_skinned_model_slots[h.index].generation == h.generation && m_skinned_model_slots[h.index].is_allocated) {
            return &m_skinned_model_slots[h.index].asset;
        }
        return nullptr;
    }

    void asset_registry::destroy_skinned_model(handle<skinned_model_asset> h) {
        if (h.index < m_skinned_model_slots.size() && m_skinned_model_slots[h.index].generation == h.generation && m_skinned_model_slots[h.index].is_allocated) {
            m_skinned_model_map.erase(m_skinned_model_slots[h.index].asset.guid);
            m_skinned_model_slots[h.index].asset.raw_data.clear_raw_data();
            m_skinned_model_slots[h.index].asset.sub_meshes.clear();
            m_skinned_model_slots[h.index].is_allocated = false;
            m_skinned_model_slots[h.index].generation++;
            m_skinned_model_free_slots.push_back(h.index);
        }
    }

    // ========================================================================
    // SİSTEM YÖNETİMİ
    // ========================================================================

    void asset_registry::clear_registry() {
        m_texture_slots.clear(); m_texture_free_slots.clear(); m_texture_map.clear();
        m_model_slots.clear(); m_model_free_slots.clear(); m_model_map.clear();
        m_material_slots.clear(); m_material_free_slots.clear(); m_material_map.clear();
        m_shader_slots.clear(); m_shader_free_slots.clear(); m_shader_map.clear();
        m_skeleton_slots.clear(); m_skeleton_free_slots.clear(); m_skeleton_map.clear();
        m_animation_slots.clear(); m_animation_free_slots.clear(); m_animation_map.clear();
        m_skinned_model_slots.clear(); m_skinned_model_free_slots.clear(); m_skinned_model_map.clear();
    }

} // namespace aria