#pragma once
#include "../Adapters/IEditableWorld.h"
#include "../CoreState/editor_selection_manager.h"
#include "../Services/asset_ui_registry.h"
#include <vector>
#include <string>

namespace vesta
{
    enum class inspector_mode
    {
        none,
        entity,
        asset
    };

    struct inspector_component_data
    {
        size_t hash = 0;
        const ComponentUIInfo* ui = nullptr;
        std::string display_name;
        bool removable = true;
        bool default_open = true;
        bool expanded = false;
    };

    struct asset_inspector_data
    {
        std::string asset_path;
        std::string asset_type;
        const AssetUIInfo* ui = nullptr;
    };

    struct add_component_entry
    {
        size_t hash = 0;
        const ComponentUIInfo* info = nullptr;
    };

    class inspector_view_model
    {
    private:
        IEditableWorld& m_world;
        registry& m_registry;
        editor_selection_manager& m_selection;
        inspector_mode m_mode = inspector_mode::none;

    public:
        inspector_view_model(
            IEditableWorld& world,
            registry& reg,
            editor_selection_manager& selection);

        void update();
        void refresh();

        inspector_mode get_mode() const;
        std::vector<add_component_entry> get_addable_components() const;
        asset_inspector_data get_asset_data() const;

        //-----------------------------------------
        // UI Katmanının Güvenle Çağıracağı Getter
        //-----------------------------------------
        registry& get_registry() const { return m_registry; }

        //-----------------------------------------
        // Selection
        //-----------------------------------------
        bool has_selected_entity() const;
        entity get_selected_entity() const;

        bool has_selected_asset() const;
        std::string get_selected_asset() const;

        //-----------------------------------------
        // Components
        //-----------------------------------------
        std::vector<inspector_component_data> get_components() const;

        //-----------------------------------------
        // Add / Remove
        //-----------------------------------------
        void request_add_component(size_t type_hash);
        void request_remove_component(size_t type_hash);

        //-----------------------------------------
        // Validation
        //-----------------------------------------
        bool can_add(size_t type_hash) const;
        bool can_remove(size_t type_hash) const;
    };
}