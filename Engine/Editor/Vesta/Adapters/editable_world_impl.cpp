/**
 * @file editable_world_impl.cpp
 * @brief IEditableWorld adaptör arayüzünün somut motor uygulaması.
 * @details ECS dünya mutasyonlarını, hiyerarşi ilişkilerini ve varlık/bileşen kayıt sistemlerini yönetir.
 */

#include "editable_world_impl.h"
#include "../../../Engine/asset_pipeline.h"
#include "../Services/component_operation_registry.h"
#include "../Services/component_ui_registry.h"
#include "../Services/editor_events.h"
#include "../../../ECS/transform_utils.h"
#include <iostream>

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
        // 1. Ebeveyn ve çocuk ilişkilerini hiyerarşik olarak çöz
        if (m_registry.has_component<hierarchy_component>(e)) {
            set_entity_parent(e, 0);

            auto* hc = m_registry.get_component<hierarchy_component>(e);
            if (hc) {
                entity current_child = hc->first_child;
                while (current_child != 0) {
                    auto* child_hc = m_registry.get_component<hierarchy_component>(current_child);
                    entity next_sibling = child_hc ? child_hc->next_sibling : 0;

                    set_entity_parent(current_child, 0);
                    current_child = next_sibling;
                }
            }
        }

        // 2. Kayıtlı tüm bileşen silme kancalarını (Observer dispatch) ve yıkım fonksiyonlarını tetikle
        const auto& operation_registry = vesta::component_operation_registry::get_registry();

        for (const auto& [hash, info] : operation_registry) {
            if (has_component_by_hash(e, hash)) {
                remove_component_by_hash(e, hash);
            }
        }

        // 3. Varlığı ECS kayıt defterinden tamamen kaldır
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
            while (ancestor != 0) {
                if (ancestor == child) {
                    return; // Döngüsel ilişki saptandı, atamayı iptal et
                }
                auto* anc_hc = m_registry.get_component<hierarchy_component>(ancestor);
                ancestor = anc_hc ? anc_hc->parent : 0;
            }
        }

        glm::mat4 old_world = ecs_transform::get_world_matrix(child, m_registry);

        // 1. Child önceden bir parent'a bağlıysa, eski ebeveynin bağlı listesinden çıkar
        if (m_registry.has_component<hierarchy_component>(child)) {
            auto* child_hc = m_registry.get_component<hierarchy_component>(child);
            if (child_hc && child_hc->parent != 0) {
                auto* old_parent_hc = m_registry.get_component<hierarchy_component>(child_hc->parent);

                if (child_hc->prev_sibling != 0) {
                    auto* prev_hc = m_registry.get_component<hierarchy_component>(child_hc->prev_sibling);
                    if (prev_hc) {
                        prev_hc->next_sibling = child_hc->next_sibling;
                    }
                }
                else if (old_parent_hc) {
                    old_parent_hc->first_child = child_hc->next_sibling;
                }

                if (child_hc->next_sibling != 0) {
                    auto* next_hc = m_registry.get_component<hierarchy_component>(child_hc->next_sibling);
                    if (next_hc) {
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

        // 4. Yeni parent'ın çocuk listesinin en başına yerleştir (Prepend)
        child_hc->parent = parent;
        child_hc->next_sibling = parent_hc->first_child;
        child_hc->prev_sibling = 0;

        if (parent_hc->first_child != 0) {
            auto* old_first_child_hc = m_registry.get_component<hierarchy_component>(parent_hc->first_child);
            if (old_first_child_hc) {
                old_first_child_hc->prev_sibling = child;
            }
        }

        parent_hc->first_child = child;

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
        // Not: Yansıtma (Reflection) veya Sera serileştirme katmanı bağlandığında uygulanacaktır.
        (void)e;
        (void)component_hash;
        return nullptr;
    }

    std::string editable_world_impl::resolve_asset_path(const aria::asset_guid& guid) const {
        if (!m_aria_system) {
            return "";
        }
        return m_aria_system->resolve_asset_path(guid);
    }

    aria::asset_guid editable_world_impl::get_guid_from_path(const std::string& path) {
        return asset_pipeline::get_or_create_meta_guid(path);
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
        return std::filesystem::exists(path);
    }

    std::optional<std::string> editable_world_impl::get_asset_type(const std::string& path) const {
        namespace fs = std::filesystem;

        if (!fs::exists(path)) {
            return std::nullopt;
        }

        const std::string ext = fs::path(path).extension().string();

        if (ext == ".mat" || ext == ".json" || ext == ".amat") {
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

        return std::nullopt;
    }

} // namespace vesta