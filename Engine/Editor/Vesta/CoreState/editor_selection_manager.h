#pragma once
#include "../../../ECS/entity.h"
#include <unordered_set>
#include <string>
#include <vector>

#include "selection_context.h"

namespace vesta {

    class editor_selection_manager {
    private:
        // Sahne nesneleri için çoklu seçim havuzu
        std::unordered_set<entity> m_selected_entities;

        // Asset/Dosya yolları veya GUID'ler için çoklu seçim havuzu
        std::unordered_set<std::string> m_selected_assets;

        // O an editörde aktif olan ana seçim bağlamı
        selection_context_type m_active_context = selection_context_type::scene_entities;

    public:
        editor_selection_manager() = default;
        ~editor_selection_manager() = default;

        // ==========================================
        // GENERAL CONTROLS
        // ==========================================
        void clear_all();
        void set_context(selection_context_type type);
        selection_context_type get_context() const { return m_active_context; }

        void clear_entity_selection();
        void clear_asset_selection();

        bool has_any_selection() const;
        bool empty() const;

        // ==========================================
        // SCENE ENTITY SELECTION API (Multi-Select)
        // ==========================================
        void select_entity(entity e, bool accumulate = false);
        void deselect_entity(entity e);

        bool is_entity_selected(entity e) const;
        entity get_first_selected_entity() const; // Tek bir nesne lazımsa (Örn: Inspector)
        const std::unordered_set<entity>& get_selected_entities() const { return m_selected_entities; }
        bool has_entity_selection() const { return !m_selected_entities.empty(); }

        // ==========================================
        // ASSET / FILE SELECTION API (Multi-Select)
        // ==========================================
        void select_asset(const std::string& asset_path_or_guid, bool accumulate = false);
        void deselect_asset(const std::string& asset_path_or_guid);

        bool is_asset_selected(const std::string& asset_path_or_guid) const;
        std::string get_first_selected_asset() const;
        const std::unordered_set<std::string>& get_selected_assets() const { return m_selected_assets; }
        bool has_asset_selection() const { return !m_selected_assets.empty(); }
    };

} // vesta