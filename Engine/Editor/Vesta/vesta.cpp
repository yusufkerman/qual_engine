// 📁 Dosya Yolu: Vesta/vesta.cpp
#include "vesta.h"

// TÜM BİLEŞEN UI SİSTEMLERİNİ BURADA TOPLUYORUZ
#include "Inspector/Components/camera_component_ui.h"
#include "Inspector/Components/collider_component_ui.h"
#include "Inspector/Components/light_component_ui.h"
#include "Inspector/Components/material_component_ui.h"
#include "Inspector/Components/mesh_component_ui.h"
#include "Inspector/Components/native_script_component_ui.h"
#include "Inspector/Components/rigidbody_component_ui.h"
#include "Inspector/Components/transform_component_ui.h"

#include "Inspector/Assets/material_inspector.h"


// Vesta Core modülü (Jolt Physics veya harici yıkım event'leri için)
#include "CoreState/vesta_module.h" 

#include "../../Systems/physics_system.h"
#include "../../Systems/physics_api.h"    

#include "../../Sera/sera.h"


namespace vesta {


    void vesta_system::init(aria::system* ariaSys, EngineContext& engineCtx, EditorContext& editorCtx)
    {
        std::cout << "[Vesta] Initializing editor ecosystem subsystems...\n";
        m_panels.clear(); // Re-init güvenliği için temizlik

        // ==========================================
        // 1. COMPONENT UI SİSTEM KAYITLARI
        // ==========================================
        register_transform_ui_system();
        register_camera_ui_system();
        register_light_ui_system();
        register_rigidbody_ui_system();
        register_collider_ui_system();
        register_native_script_ui_system();
        register_material_ui_system(*engineCtx.aria_system);
        register_mesh_ui_system(*engineCtx.aria_system);

        // ==========================================
        // 2. ÇEKİRDEK SERVİSLERİN BAŞLATILMASI
        // ==========================================
        registry& reg = engineCtx.session.game_scene().get_registry();

        m_editable_world = std::make_unique<editable_world_impl>(reg, ariaSys);
        m_command_stack = std::make_unique<command_stack>(*m_editable_world);
        m_material_service = std::make_unique<material_service>(*m_editable_world, *m_command_stack);
        m_selection_manager = std::make_unique<editor_selection_manager>();

        // ==========================================
        // 1. ASSET UI SİSTEM KAYITLARI
        // ==========================================
        register_material_asset_ui_system(engineCtx.aria_system, m_material_service.get());

        // ==========================================
        // 3. MODÜLER PANEL & VIEWMODEL BAŞLATMALARI
        // ==========================================
        init_top_bar(engineCtx, editorCtx);
        init_scene_viewport(engineCtx);
        init_game_viewport(engineCtx);
        init_hierarchy(reg);
        init_inspector(reg);
        init_asset_browser(*ariaSys);

        // ==========================================
        // 4. PHYSICS API HOOKS
        // ==========================================
        auto& physApi = engineCtx.physicsSystem.api();
        apply_hooks(physApi);
    }
    void vesta_system::register_panel(IEditorPanel* panel)
    {
        if (panel) {
            m_panels.push_back(panel);
        }
    }
	void vesta_system::apply_hooks(physics_api& api)
	{
        vesta::vesta_module::register_remove_hook<rigidbody_component>(
            [&api](entity e)
            {
                api.destroy_body(e);
            });

        vesta::vesta_module::register_remove_hook<collider_component>(
            [&api](entity e)
            {
                api.remove_collider_shape(e);
            });
	}
    void vesta_system::init_top_bar(
        EngineContext& engineCtx,
        EditorContext& editorCtx
    )
    {
        m_top_bar_vm =
            std::make_unique<top_bar_view_model>(
                editorCtx,

                [&engineCtx](const std::string& path)
                {
                    if (engineCtx.sera)
                    {
                        engineCtx.sera->save_scene(
                            path,
                            engineCtx.session.editor_scene()
                        );
                    }
                },

                [&engineCtx](const std::string& path)
                {
                    if (engineCtx.sera)
                    {
                        engineCtx.sera->load_scene(
                            path,
                            engineCtx.session.editor_scene()
                        );
                    }
                }
            );


        m_top_bar_panel =
            std::make_unique<top_bar_panel>(
                *m_top_bar_vm
            );

        register_panel(
            m_top_bar_panel.get()
        );
    }
    void vesta_system::init_scene_viewport(EngineContext& ctx)
    {
        m_scene_viewport_vm =
            std::make_unique<scene_viewport_view_model>(m_viewport_state);

        if (m_scene_viewport_vm)
            m_scene_viewport_vm->set_texture_id(ctx.editorTarget.tex);

        m_scene_viewport_panel = std::make_unique<scene_viewport_panel>(*m_scene_viewport_vm, m_viewport_state);
        register_panel(m_scene_viewport_panel.get());
    }

    void vesta_system::init_game_viewport(EngineContext& ctx)
    {
        m_game_viewport_vm =
            std::make_unique<game_viewport_view_model>(m_viewport_state);

        if (m_game_viewport_vm)
            m_game_viewport_vm->set_texture_id(ctx.gameTarget.tex);

        m_game_viewport_panel = std::make_unique<game_viewport_panel>(*m_game_viewport_vm, m_viewport_state);
        register_panel(m_game_viewport_panel.get());
    }
    void vesta_system::init_hierarchy(registry& registry)
    {
        m_hierarchy_panel = std::make_unique<hierarchy_panel>(
            *m_editable_world,
            registry,
            *m_selection_manager
        );

        register_panel(m_hierarchy_panel.get());

    }
    void vesta_system::init_inspector(registry& reg)
    {
        m_inspector_vm = std::make_unique<inspector_view_model>(
            *m_editable_world,
            reg,
            *m_selection_manager
        );

        m_inspector_panel = std::make_unique<inspector_panel>(std::move(m_inspector_vm));

        register_panel(m_inspector_panel.get());

    }
    void vesta_system::init_asset_browser(aria::system& ariaSys)
    {
        // Asset browser sistemi
        m_asset_browser_system = std::make_unique<asset_browser_system>(
            "Assets",
            &ariaSys,
            m_material_service.get()
        );

        m_asset_browser_system->refresh();

        // ViewModel
        m_asset_browser_vm = std::make_unique<asset_browser_view_model>(
            *m_asset_browser_system,
            *m_material_service,

            [this](const std::string& path)
            {
                if (path.empty())
                    m_selection_manager->clear_asset_selection();
                else
                    m_selection_manager->select_asset(path);
            },

            [this]()
            {
                m_selection_manager->clear_entity_selection();
            }
        );

        // Panel
        m_asset_browser_panel =
            std::make_unique<asset_browser_panel>(*m_asset_browser_vm);

        m_asset_browser_panel->init();

        register_panel(m_asset_browser_panel.get());

    }
    void vesta_system::update()
    {
        for (auto* panel : m_panels)
        {
            if (panel && panel->is_open())
            {
                panel->render();
            }
        }
    }
} // namespace vesta