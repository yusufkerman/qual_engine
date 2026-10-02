/**
 * @file hierarchy_view_model.cpp
 * @brief Sahne Hiyerarşisi (Hierarchy) görünüm modelinin mantıksal uygulaması.
 */

#include "hierarchy_view_model.h"

namespace vesta {

    hierarchy_view_model::hierarchy_view_model(IEditableWorld& world, registry& reg, editor_selection_manager& selection)
        : m_world(world), m_registry(reg), m_selection_manager(selection) {
    }

    void hierarchy_view_model::gather_children_recursive(entity parent, std::vector<entity>& out_children) const {
        if (auto* hc = m_registry.get_component<hierarchy_component>(parent)) {
            entity current_child = hc->first_child;

            // Kardeş (sibling) zincirini dolaşarak tüm doğrudan çocukları topla
            while (current_child != 0) {
                out_children.push_back(current_child);
                if (auto* child_hc = m_registry.get_component<hierarchy_component>(current_child)) {
                    current_child = child_hc->next_sibling;
                }
                else {
                    break;
                }
            }
        }
    }

    hierarchy_node_data hierarchy_view_model::get_node_data(entity e) const {
        hierarchy_node_data data;
        data.id = e;
        data.is_selected = m_selection_manager.is_entity_selected(e);

        if (auto* name_comp = m_registry.get_component<name_component>(e)) {
            data.name = name_comp->name.empty() ? "Unnamed Entity" : name_comp->name;
        }
        else {
            data.name = "Entity " + std::to_string(e);
        }

        if (auto* hc = m_registry.get_component<hierarchy_component>(e)) {
            data.parent_id = hc->parent;
            data.has_children = (hc->first_child != 0);
            gather_children_recursive(e, data.children);
        }

        return data;
    }

    std::vector<hierarchy_node_data> hierarchy_view_model::get_root_nodes() const {
        std::vector<hierarchy_node_data> root_nodes;
        const auto& all_entities = m_registry.get_all_entities();

        for (entity e : all_entities) {
            bool is_root = true;
            if (auto* hc = m_registry.get_component<hierarchy_component>(e)) {
                if (hc->parent != 0) {
                    is_root = false;
                }
            }

            if (is_root) {
                root_nodes.push_back(get_node_data(e));
            }
        }
        return root_nodes;
    }

    // ==========================================
    // MUTATION DISPATCHERS (YAZMA İŞLEMLERİ)
    // ==========================================

    void hierarchy_view_model::request_create_entity(const std::string& default_name) {
        entity e = m_world.spawn_entity(default_name);
        m_selection_manager.select_entity(e, false);
        start_rename(e);
    }

    void hierarchy_view_model::request_destroy_entity(entity e) {
        if (m_selection_manager.is_entity_selected(e)) {
            m_selection_manager.deselect_entity(e);
        }
        m_world.destroy_entity(e);
    }

    void hierarchy_view_model::request_set_parent(entity child, entity parent) {
        m_world.set_entity_parent(child, parent);
    }

    void hierarchy_view_model::request_rename_entity(entity e, const std::string& new_name) {
        if (auto* n = m_registry.get_component<name_component>(e)) {
            n->name = new_name;
            m_world.mark_scene_dirty();
        }
        m_renaming_entity = 0;
    }

    void hierarchy_view_model::request_select(entity e, bool accumulate) {
        m_selection_manager.select_entity(e, accumulate);
    }

    void hierarchy_view_model::request_copy(entity e) {
        if (e != 0) {
            m_clipboard_entity = e;
        }
    }

    void hierarchy_view_model::request_paste() {
        if (m_clipboard_entity == 0) return;

        entity duplicated = m_registry.duplicate_entity(m_clipboard_entity);
        m_selection_manager.select_entity(duplicated, false);
        m_world.mark_scene_dirty();
    }

    void hierarchy_view_model::request_duplicate(entity e) {
        if (e == 0) return;

        entity duplicated = m_registry.duplicate_entity(e);

        if (auto* hc = m_registry.get_component<hierarchy_component>(e)) {
            m_world.set_entity_parent(duplicated, hc->parent);
        }

        m_selection_manager.select_entity(duplicated, false);
        m_world.mark_scene_dirty();
    }

    // ==========================================
    // SELECTION (SEÇİM DURUMU)
    // ==========================================

    bool hierarchy_view_model::has_selection() const {
        return m_selection_manager.has_entity_selection();
    }

    entity hierarchy_view_model::get_selected_entity() const {
        return m_selection_manager.get_first_selected_entity();
    }

    bool hierarchy_view_model::is_selected(entity e) const {
        return m_selection_manager.is_entity_selected(e);
    }

    // ==========================================
    // RENAME STATE (YENİDEN ADLANDIRMA)
    // ==========================================

    bool hierarchy_view_model::is_renaming(entity e) const {
        return m_renaming_entity == e;
    }

    void hierarchy_view_model::start_rename(entity e) {
        m_renaming_entity = e;
        m_rename_buffer.clear();

        if (auto* name = m_registry.get_component<name_component>(e)) {
            m_rename_buffer = name->name;
        }
    }

    void hierarchy_view_model::commit_rename() {
        if (m_renaming_entity == 0) return;

        request_rename_entity(m_renaming_entity, m_rename_buffer);
        m_renaming_entity = 0;
    }

    void hierarchy_view_model::cancel_rename() {
        m_renaming_entity = 0;
        m_rename_buffer.clear();
    }

    entity hierarchy_view_model::get_renaming_entity() const {
        return m_renaming_entity;
    }

    std::string& hierarchy_view_model::get_rename_buffer() {
        return m_rename_buffer;
    }

    const std::string& hierarchy_view_model::get_rename_buffer() const {
        return m_rename_buffer;
    }

    // ==========================================
    // CLIPBOARD (PANO DURUMU)
    // ==========================================

    bool hierarchy_view_model::has_clipboard() const {
        return m_clipboard_entity != 0;
    }

    // ==========================================
    // VALIDATION (DOĞRULAMA)
    // ==========================================

    bool hierarchy_view_model::can_copy() const {
        return has_selection();
    }

    bool hierarchy_view_model::can_duplicate() const {
        return has_selection();
    }

    bool hierarchy_view_model::can_delete() const {
        return has_selection();
    }

    bool hierarchy_view_model::can_paste() const {
        return has_clipboard();
    }

    // ==========================================
    // HIGH LEVEL ACTIONS (YÜKSEK SEVİYELİ İŞLEMLER)
    // ==========================================

    void hierarchy_view_model::request_copy_selected() {
        entity selected = get_selected_entity();
        if (selected != 0) {
            request_copy(selected);
        }
    }

    void hierarchy_view_model::request_duplicate_selected() {
        entity selected = get_selected_entity();
        if (selected != 0) {
            request_duplicate(selected);
        }
    }

    void hierarchy_view_model::request_delete_selected() {
        entity selected = get_selected_entity();
        if (selected != 0) {
            request_destroy_entity(selected);
        }
    }

} // namespace vesta