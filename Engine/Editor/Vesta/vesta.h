#pragma once

/**
 * @file vesta.h
 * @brief Vesta Editör Sistemini tanımlar. Editör panellerini, görünümleri ve seçim durumlarını yöneten merkezi sınıf.
 */

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
#include "Panels/lighting_panel.h"
#include "Panels/rendering_panel.h"
#include "../../ECS/registry.h"
#include "CoreState/editor_viewport_state.h"

struct EngineContext;
struct EditorContext;
class IEditorPanel;
class editor_selection_manager;
class physics_api;
class command_stack;

namespace aria {
    class system;
}

namespace vesta {

    /**
     * @class vesta_system
     * @brief Oyun motorunun editör arayüzünü (UI), panellerini ve MVVM yapılarını başlatan ve güncelleyen ana sistem.
     */
    class vesta_system
    {
    public:
        /**
         * @brief Tüm editör alt sistemlerini, panellerini ve görünüm modellerini (ViewModel) başlatır.
         *
         * @param ariaSys Aria varlık sistemi işaretçisi.
         * @param engineCtx Motor bağlam (context) nesnesi.
         * @param editorCtx Editör bağlam (context) nesnesi.
         */
        void init(aria::system* ariaSys, EngineContext& engineCtx, EditorContext& editorCtx);

        /**
         * @brief Tüm panelleri, ViewModel ve servisleri bellekten temizler.
         */
        void shutdown();

        /**
         * @brief Kayıtlı tüm editör panellerinin render döngülerini tetikler.
         */
        void update();

        /**
         * @brief Yeni bir editör panelini sisteme kaydeder.
         * @param panel Sisteme eklenecek panelin arayüz işaretçisi.
         */
        void register_panel(IEditorPanel* panel);

        // --- GÖRÜNÜM VE SEÇİM DURUMLARINA (STATE) ERİŞİM ---

        editor_viewport_state& viewport_state() { return m_viewport_state; }
        const editor_viewport_state& viewport_state() const { return m_viewport_state; }

        editor_selection_manager& selection() { return *m_selection_manager; }
        const editor_selection_manager& selection() const { return *m_selection_manager; }

        /**
         * @brief Varlık tarayıcısının kök dizinini günceller (Örn: Proje değiştirildiğinde).
         */
        void set_asset_root(const std::string& newPath);

        /**
         * @brief Pencerenin en üstünde yer alan ana menü çubuğunu (File, Window) çizer.
         */
        void render_main_menu_bar(EngineContext& engineCtx, EditorContext& editorCtx);

        /**
         * @brief Başlığı verilen paneli kayıtlı paneller arasında arar.
         */
        IEditorPanel* find_panel(const std::string& title) const;

    private:

        /**
         * @brief Fizik sistemi ve diğer çekirdek modüller için yıkım (destroy) kancalarını kaydeder.
         */
        void apply_hooks(physics_api& api);

        void init_top_bar(EngineContext& engineCtx, EditorContext& editorCtx);
        void init_scene_viewport(EngineContext& engineCtx, EditorContext& editorCtx);
        void init_game_viewport(EngineContext& ctx);
        void init_inspector(registry& registry, EngineContext& engineCtx);
        void init_hierarchy(registry& registry, EngineContext& engineCtx);
        void init_asset_browser(aria::system& ariaSys);
        void init_lighting(EngineContext& engineCtx);
        void init_rendering_settings(EngineContext& engineCtx);

    private:
        editor_viewport_state m_viewport_state;

        /// Polimorfik panel arayüzlerinin tutulduğu liste
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

        std::unique_ptr<lighting_panel> m_lighting_panel;
        std::unique_ptr<rendering_panel> m_rendering_panel;

        std::unique_ptr<command_stack> m_command_stack;
        std::unique_ptr<editable_world_impl> m_editable_world;
    };

} // namespace vesta