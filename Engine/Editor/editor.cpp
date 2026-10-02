//#include "editor.h"
//#include "../Scene/scene.h"
//#include "../Engine/engine_context.h"
//#include <imgui.h>
//#include <string>
//#include <imgui_internal.h>
//#include "Panels/asset_browser_panel.h"
//
//// ========================================================================
//// LIFECYCLE (CONSTRUCTOR & DESTRUCTOR)
//// ========================================================================
//Editor::Editor()
//{
//    assetBrowserPanel = nullptr;
//}
//
//Editor::~Editor()
//{
//    if (assetBrowserPanel)
//    {
//        delete assetBrowserPanel;
//        assetBrowserPanel = nullptr;
//    }
//}
//
//void Editor::init(GLFWwindow*)
//{
//}
//
//void Editor::update(scene&) {}
//
//// ========================================================================
//// MAIN UI RENDERING
//// ========================================================================
//void Editor::renderUI(scene& scene, EngineContext& ctx, EditorContext& editorCtx)
//{
//    if (assetBrowserPanel == nullptr && ctx.aria_system != nullptr)
//    {
//        assetBrowserPanel = new aria::editor::asset_browser_panel(ctx.aria_system);
//        assetBrowserPanel->init();
//    }
//
//    auto& reg = scene.get_registry();
//
//    editorCtx.focusedPanel = FocusedPanel::None;
//
//    // --- Top Bar ---
//    topBarPanel.render(mode, scene, ctx);
//
//    // --- Hierarchy ---
//    hierarchyPanel.render(
//        scene,
//        selectedEntity,
//        renamingEntity,
//        renameBuffer,
//        256,
//        deleteQueue,
//        editorCtx);
//
//    // --- Scene Viewport ---
//    sceneViewportPanel.render(
//        sceneTexture,
//        sceneViewportSize,
//        sceneViewportHovered,
//        editorCamera,
//        editorCtx);
//
//    // --- Game Viewport ---
//    gameViewportPanel.render(
//        gameTexture,
//        gameViewportSize,
//        gameViewportHovered,
//        editorCtx);
//
//    if (assetBrowserPanel != nullptr)
//    {
//        editorCtx.selectedAssetPath = assetBrowserPanel->get_selected_path();
//    }
//    else
//    {
//        editorCtx.selectedAssetPath = "";
//    }
//
//    // --- Inspector ---
//    inspectorPanel.render(
//        scene,
//        selectedEntity,
//        ctx,
//        editorCtx);
//
//    // --- Asset Browser ---
//    if (assetBrowserPanel != nullptr)
//    {
//        assetBrowserPanel->render(editorCtx);
//    }
//
//    // --- Entity Deletion Processing ---
//    if (!deleteQueue.empty())
//    {
//        auto& sceneEntities = const_cast<std::vector<entity>&>(scene.get_entities());
//
//        for (entity e : deleteQueue)
//        {
//            if (selectedEntity == (int)e)
//                selectedEntity = -1;
//
//            if (renamingEntity == (int)e)
//                renamingEntity = -1;
//
//            reg.remove_entity(e);
//
//            sceneEntities.erase(
//                std::remove(sceneEntities.begin(), sceneEntities.end(), e),
//                sceneEntities.end()
//            );
//        }
//
//        deleteQueue.clear();
//    }
//}