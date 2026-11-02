/**
 * @file editable_world_impl.cpp
 * @brief IEditableWorld adaptör arayüzünün somut motor uygulaması.
 * @details ECS dünya mutasyonlarını, hiyerarşi ilişkilerini ve varlık/bileşen kayıt sistemlerini yönetir.
 */

#include "editable_world_impl.h"
#include "../../../Core/path_utils.h"
#include "../../../Engine/asset_pipeline.h"
#include <fstream>
#include "../Services/component_operation_registry.h"
#include "../Services/component_ui_registry.h"
#include "../Services/editor_events.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"
#include <iostream>
#include <typeinfo>
#include <filesystem>
#include <algorithm>

namespace vesta {

    editable_world_impl::editable_world_impl(registry& reg, aria::system* aria_sys)
        : m_registry(reg), m_aria_system(aria_sys) {
    }

    entity editable_world_impl::spawn_entity(const std::string& name) {
        entity e = m_registry.create_entity();

        m_registry.add_component<name_component>(e, name_component{ name });
        m_registry.add_component<transform_component>(e, transform_component{});

        mark_scene_dirty();
        return e;
    }

    void editable_world_impl::destroy_entity(entity e) {
        if (!m_registry.valid(e)) {
            return;
        }

        // 1. Ebeveyn ve çocuk ilişkilerini hiyerarşik olarak çöz
        // Bir parent silindiğinde altındaki tüm çocuk nesneler de özyinelemeli olarak silinir
        std::vector<entity> children;
        if (m_registry.has_component<hierarchy_component>(e)) {
            auto* hc = m_registry.get_component<hierarchy_component>(e);
            if (hc) {
                entity current_child = hc->first_child;
                int safety = 0;
                std::unordered_set<entity> visited;
                while (current_child != 0 && safety++ < 100000) {
                    if (visited.contains(current_child)) {
                        break;
                    }
                    visited.insert(current_child);
                    children.push_back(current_child);
                    auto* child_hc = m_registry.get_component<hierarchy_component>(current_child);
                    current_child = child_hc ? child_hc->next_sibling : 0;
                }
            }

            // Kendisini ebeveyninden ayır
            set_entity_parent(e, 0);
        }

        // 2. Altındaki tüm çocuk nesneleri özyinelemeli (recursive) olarak yok et
        for (entity child : children) {
            if (m_registry.valid(child)) {
                destroy_entity(child);
            }
        }

        // 3. Kayıtlı tüm bileşen silme kancalarını (Observer dispatch) ve yıkım fonksiyonlarını tetikle
        const auto& operation_registry = vesta::component_operation_registry::get_registry();

        for (const auto& [hash, info] : operation_registry) {
            if (has_component_by_hash(e, hash)) {
                remove_component_by_hash(e, hash);
            }
        }

        // 4. Varlığı ECS kayıt defterinden tamamen kaldır
        m_registry.remove_entity(e);
        mark_scene_dirty();
    }

    void editable_world_impl::set_entity_parent(entity child, entity parent) {
        if (child == parent || child == 0) {
            return;
        }

        // Döngüsel hiyerarşi engelleme: Hedef parent, çocuğun soyundan geliyor mu?
        if (parent != 0 && m_registry.has_component<hierarchy_component>(parent)) {
            entity ancestor = parent;
            int safety = 0;
            while (ancestor != 0 && safety++ < 100000) {
                if (ancestor == child) {
                    return; // Döngüsel ilişki saptandı, atamayı iptal et
                }
                auto* anc_hc = m_registry.get_component<hierarchy_component>(ancestor);
                ancestor = anc_hc ? anc_hc->parent : 0;
            }
        }

        glm::mat4 old_world = ecs_transform::get_world_matrix(child, m_registry);

        // 1. Child önceden bir parent'a bağlıysa, eski ebeveynin bağlı listesinden güvenli şekilde çıkar
        if (m_registry.has_component<hierarchy_component>(child)) {
            auto* child_hc = m_registry.get_component<hierarchy_component>(child);
            if (child_hc && child_hc->parent != 0) {
                auto* old_parent_hc = m_registry.get_component<hierarchy_component>(child_hc->parent);

                if (child_hc->prev_sibling != 0) {
                    auto* prev_hc = m_registry.get_component<hierarchy_component>(child_hc->prev_sibling);
                    if (prev_hc && prev_hc->next_sibling == child) {
                        prev_hc->next_sibling = child_hc->next_sibling;
                    }
                }
                else if (old_parent_hc && old_parent_hc->first_child == child) {
                    old_parent_hc->first_child = child_hc->next_sibling;
                }

                if (child_hc->next_sibling != 0) {
                    auto* next_hc = m_registry.get_component<hierarchy_component>(child_hc->next_sibling);
                    if (next_hc && next_hc->prev_sibling == child) {
                        next_hc->prev_sibling = child_hc->prev_sibling;
                    }
                }

                child_hc->parent = 0;
                child_hc->next_sibling = 0;
                child_hc->prev_sibling = 0;
            }
        }

        // 2. Yeni atanan parent kök dizin ise (parent == 0) dünya koordinatlarını koru ve çık
        if (parent == 0) {
            auto* t = m_registry.get_component<transform_component>(child);
            if (t) {
                ecs_transform::apply_world_matrix_as_local(*t, old_world);
            }
            mark_scene_dirty();
            return;
        }

        // 3. Parent ve Child için hiyerarşi bileşenlerini doğrula
        if (!m_registry.has_component<hierarchy_component>(parent)) {
            m_registry.add_component<hierarchy_component>(parent);
        }
        if (!m_registry.has_component<hierarchy_component>(child)) {
            m_registry.add_component<hierarchy_component>(child);
        }

        auto* child_hc = m_registry.get_component<hierarchy_component>(child);
        auto* parent_hc = m_registry.get_component<hierarchy_component>(parent);

        // 4. Yeni parent'ın çocuk listesinin sonuna yerleştir (Append)
        child_hc->parent = parent;
        child_hc->next_sibling = 0;

        if (parent_hc->first_child == 0) {
            child_hc->prev_sibling = 0;
            parent_hc->first_child = child;
        }
        else {
            entity tail = parent_hc->first_child;
            int safety = 0;
            while (tail != 0 && safety++ < 100000) {
                if (tail == child) {
                    break;
                }
                auto* tail_hc = m_registry.get_component<hierarchy_component>(tail);
                if (!tail_hc || tail_hc->next_sibling == 0) {
                    break;
                }
                tail = tail_hc->next_sibling;
            }
            if (tail != 0 && tail != child) {
                if (auto* tail_hc = m_registry.get_component<hierarchy_component>(tail)) {
                    tail_hc->next_sibling = child;
                    child_hc->prev_sibling = tail;
                }
            }
        }

        // 5. Dünya konumunu korumak için yeni ebeveyne göre yerel dönüşüm matrisini hesapla
        glm::mat4 new_parent_world = ecs_transform::get_world_matrix(parent, m_registry);
        glm::mat4 new_local = glm::inverse(new_parent_world) * old_world;

        auto* t = m_registry.get_component<transform_component>(child);
        if (t) {
            ecs_transform::apply_world_matrix_as_local(*t, new_local);
        }

        mark_scene_dirty();
    }

    void editable_world_impl::add_component_by_hash(entity e, size_t component_hash) {
        const auto& registry = component_operation_registry::get_registry();
        auto it = registry.find(component_hash);

        if (it == registry.end()) {
            std::cerr << "[Vesta EditableWorld] Bilesen operasyonu bulunamadi: " << component_hash << '\n';
            return;
        }

        if (it->second.add_fn) {
            it->second.add_fn(e, m_registry);
            mark_scene_dirty();
        }
    }

    void editable_world_impl::remove_component_by_hash(entity e, size_t component_hash) {
        // Dış sistemlerin (Fizik, Ses) bileşen yok edilmeden önce haberdar olması için olay dağıtımı yap
        editor_events::dispatch_on_remove(component_hash, e, m_registry);

        const auto& registry = component_operation_registry::get_registry();
        auto it = registry.find(component_hash);

        if (it == registry.end()) {
            return;
        }

        if (it->second.remove_fn) {
            it->second.remove_fn(e, m_registry);
            mark_scene_dirty();
        }
    }

    bool editable_world_impl::has_component_by_hash(entity e, size_t component_hash) const {
        const auto& registry = component_operation_registry::get_registry();
        auto it = registry.find(component_hash);

        if (it == registry.end() || !it->second.has_fn) {
            return false;
        }

        return it->second.has_fn(e, m_registry);
    }

    void* editable_world_impl::get_component_raw(entity e, size_t component_hash) {
        if (!m_registry.valid(e)) return nullptr;

        if (component_hash == typeid(transform_component).hash_code()) return m_registry.get_component<transform_component>(e);
        if (component_hash == typeid(name_component).hash_code()) return m_registry.get_component<name_component>(e);
        if (component_hash == typeid(mesh_component).hash_code()) return m_registry.get_component<mesh_component>(e);
        if (component_hash == typeid(camera_component).hash_code()) return m_registry.get_component<camera_component>(e);
        if (component_hash == typeid(light_component).hash_code()) return m_registry.get_component<light_component>(e);
        if (component_hash == typeid(material_component).hash_code()) return m_registry.get_component<material_component>(e);
        if (component_hash == typeid(bounds_component).hash_code()) return m_registry.get_component<bounds_component>(e);
        if (component_hash == typeid(rigidbody_component).hash_code()) return m_registry.get_component<rigidbody_component>(e);
        if (component_hash == typeid(collider_component).hash_code()) return m_registry.get_component<collider_component>(e);
        if (component_hash == typeid(native_script_component).hash_code()) return m_registry.get_component<native_script_component>(e);
        if (component_hash == typeid(hierarchy_component).hash_code()) return m_registry.get_component<hierarchy_component>(e);
        if (component_hash == typeid(skinned_mesh_component).hash_code()) return m_registry.get_component<skinned_mesh_component>(e);
        if (component_hash == typeid(skeleton_component).hash_code()) return m_registry.get_component<skeleton_component>(e);
        if (component_hash == typeid(animator_component).hash_code()) return m_registry.get_component<animator_component>(e);
        if (component_hash == typeid(bone_attachment_component).hash_code()) return m_registry.get_component<bone_attachment_component>(e);
        if (component_hash == typeid(canvas_component).hash_code()) return m_registry.get_component<canvas_component>(e);
        if (component_hash == typeid(rect_transform_component).hash_code()) return m_registry.get_component<rect_transform_component>(e);
        if (component_hash == typeid(ui_text_component).hash_code()) return m_registry.get_component<ui_text_component>(e);
        if (component_hash == typeid(ui_image_component).hash_code()) return m_registry.get_component<ui_image_component>(e);
        if (component_hash == typeid(ui_button_component).hash_code()) return m_registry.get_component<ui_button_component>(e);
        if (component_hash == typeid(audio_source_component).hash_code()) return m_registry.get_component<audio_source_component>(e);
        if (component_hash == typeid(audio_listener_component).hash_code()) return m_registry.get_component<audio_listener_component>(e);

        return nullptr;
    }

    std::string editable_world_impl::resolve_asset_path(const aria::asset_guid& guid) const {
        if (!m_aria_system) {
            return "";
        }
        return m_aria_system->resolve_asset_path(guid);
    }

    aria::asset_guid editable_world_impl::get_guid_from_path(const std::string& path) {
        aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(path);
        if (m_aria_system && !guid.is_nil()) {
            m_aria_system->register_asset_path(guid, path);
        }
        return guid;
    }

    const ComponentUIInfo* editable_world_impl::get_component_ui(size_t hash) const {
        const auto& registry = vesta::component_ui_registry::get_registry();
        auto it = registry.find(hash);

        if (it == registry.end()) {
            return nullptr;
        }

        return &it->second;
    }

    const std::unordered_map<size_t, ComponentUIInfo>& editable_world_impl::get_component_registry() const {
        return vesta::component_ui_registry::get_registry();
    }

    const AssetUIInfo* editable_world_impl::get_asset_ui(const std::string& type) const {
        const auto& registry = asset_ui_registry::get_registry();
        auto it = registry.find(type);

        if (it == registry.end()) {
            return nullptr;
        }

        return &it->second;
    }

    const std::unordered_map<std::string, AssetUIInfo>& editable_world_impl::get_asset_registry() const {
        return asset_ui_registry::get_registry();
    }

    bool editable_world_impl::asset_exists(const std::string& path) const {
        return std::filesystem::exists(qual::path_from_utf8(path));
    }

    std::optional<std::string> editable_world_impl::get_asset_type(const std::string& path) const {
        namespace fs = std::filesystem;

        const fs::path fs_path = qual::path_from_utf8(path);
        if (!fs::exists(fs_path)) {
            return std::nullopt;
        }

        std::string ext = fs_path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".mat" || ext == ".amat") {
            return "material";
        }

        if (ext == ".json") {
            // Materyal json'u ile sahne json'unu ayırt et
            std::ifstream f(fs_path);
            if (f.is_open()) {
                std::string line;
                for (int i = 0; i < 15 && std::getline(f, line); ++i) {
                    if (line.find("material") != std::string::npos ||
                        line.find("base_color") != std::string::npos ||
                        line.find("shader_guid") != std::string::npos ||
                        line.find("roughness") != std::string::npos) {
                        return "material";
                    }
                    if (line.find("entities") != std::string::npos ||
                        line.find("scene_name") != std::string::npos) {
                        return "scene";
                    }
                }
            }
            return "material";
        }

        if (ext == ".mesh") {
            return "mesh";
        }

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".dds") {
            return "texture";
        }

        if (ext == ".prefab") {
            return "prefab";
        }

        if (ext == ".scene") {
            return "scene";
        }

        if (ext == ".shader") {
            return "shader";
        }

        if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
            return "audio";
        }

        return std::nullopt;
    }

} // namespace vesta
