/**
 * @file editor_selection_manager.cpp
 * @brief Editör seçim yöneticisi mantıksal uygulaması.
 */

#include "editor_selection_manager.h"

namespace vesta {

    void editor_selection_manager::clear_all() {
        m_selected_entities.clear();
        m_selected_assets.clear();
    }

    void editor_selection_manager::set_context(selection_context_type type) {
        if (m_active_context != type) {
            m_active_context = type;
        }
    }

    bool editor_selection_manager::has_any_selection() const {
        return has_entity_selection() || has_asset_selection();
    }

    bool editor_selection_manager::empty() const {
        return !has_any_selection();
    }

    void editor_selection_manager::clear_entity_selection() {
        m_selected_entities.clear();
    }

    void editor_selection_manager::clear_asset_selection() {
        m_selected_assets.clear();
    }

    // ==========================================
    // ENTITY IMPLEMENTATION
    // ==========================================

    void editor_selection_manager::select_entity(entity e, bool accumulate) {
        set_context(selection_context_type::scene_entities);

        if (!accumulate) {
            m_selected_entities.clear();
        }

        if (e != 0) {
            m_selected_entities.insert(e);
        }
    }

    void editor_selection_manager::deselect_entity(entity e) {
        m_selected_entities.erase(e);
    }

    bool editor_selection_manager::is_entity_selected(entity e) const {
        return m_selected_entities.find(e) != m_selected_entities.end();
    }

    entity editor_selection_manager::get_first_selected_entity() const {
        if (m_selected_entities.empty()) {
            return 0;
        }
        return *m_selected_entities.begin();
    }

    // ==========================================
    // ASSET IMPLEMENTATION
    // ==========================================

    void editor_selection_manager::select_asset(const std::string& asset_path_or_guid, bool accumulate) {
        set_context(selection_context_type::asset_files);

        if (!accumulate) {
            m_selected_assets.clear();
        }

        if (!asset_path_or_guid.empty()) {
            m_selected_assets.insert(asset_path_or_guid);
        }
    }

    void editor_selection_manager::deselect_asset(const std::string& asset_path_or_guid) {
        m_selected_assets.erase(asset_path_or_guid);
    }

    bool editor_selection_manager::is_asset_selected(const std::string& asset_path_or_guid) const {
        return m_selected_assets.find(asset_path_or_guid) != m_selected_assets.end();
    }

    std::string editor_selection_manager::get_first_selected_asset() const {
        if (m_selected_assets.empty()) {
            return "";
        }
        return *m_selected_assets.begin();
    }

} // namespace vesta