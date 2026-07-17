#pragma once
#include "Services/material_service.h"
#include "Adapters/editable_world_impl.h"

#include "Services/asset_browser_system.h"
#include "ViewModels/asset_browser_view_model.h"
#include "Panels/asset_browser_panel.h"

#include "ViewModels/inspector_view_model.h"
#include "Panels/inspector_panel.h"

#include "ViewModels/hierarchy_view_model.h"
#include "Panels/hierarchy_panel.h"

#include "ViewModels/game_viewport_view_model.h"
#include "Panels/game_viewport_panel.h"

#include "Panels/scene_viewport_panel.h"
#include "ViewModels/scene_viewport_view_model.h"

#include "ViewModels/top_bar_view_model.h"
#include "Panels/top_bar_panel.h"

#include "../../ECS/registry.h"
#include "CoreState/editor_viewport_state.h"



class EngineContext;
struct EditorContext;

namespace aria { class system; }

class IEditorPanel;
class editor_selection_manager;
class physics_api;
class command_stack;

namespace vesta {

    class vesta_system
    {
    public:
        void init(aria::system* ariaSys, EngineContext& engineCtx, EditorContext& editorCtx);

        void update();
        void register_panel(IEditorPanel* panel);

        editor_viewport_state& viewport_state()
        {
            return m_viewport_state;
        }

        const editor_viewport_state& viewport_state() const
        {
            return m_viewport_state;
        }

    private:

        void apply_hooks(physics_api& api);

        void init_top_bar(EngineContext& engineCtx, EditorContext& editorCtx);
        void init_scene_viewport(EngineContext& ctx);
        void init_game_viewport(EngineContext& ctx);
        void init_inspector(registry& registry);
		void init_hierarchy(registry& registry);
        void init_asset_browser(aria::system& ariaSys);

        editor_viewport_state m_viewport_state;

        // Polimorfik panel listesi
        std::vector<IEditorPanel*> m_panels;

        std::unique_ptr<editor_selection_manager> m_selection_manager;

        std::unique_ptr<material_service> m_material_service;

        std::unique_ptr<asset_browser_system> m_asset_browser_system;
        std::unique_ptr<asset_browser_view_model> m_asset_browser_vm;
        std::unique_ptr<asset_browser_panel> m_asset_browser_panel;

		std::unique_ptr<inspector_view_model> m_inspector_vm;
		std::unique_ptr<inspector_panel> m_inspector_panel;

		std::unique_ptr<hierarchy_panel> m_hierarchy_panel;

        std::unique_ptr<game_viewport_view_model> m_game_viewport_vm;
        std::unique_ptr<game_viewport_panel> m_game_viewport_panel;

        std::unique_ptr<scene_viewport_view_model> m_scene_viewport_vm;
        std::unique_ptr<scene_viewport_panel> m_scene_viewport_panel;

        std::unique_ptr<top_bar_view_model> m_top_bar_vm;
        std::unique_ptr<top_bar_panel> m_top_bar_panel;

        std::unique_ptr<command_stack> m_command_stack;
        std::unique_ptr<editable_world_impl> m_editable_world;

    };

}