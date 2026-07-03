#include "editor.h"
#include "../Scene/scene.h"
#include "../Engine/engine_context.h"
#include <imgui.h>
#include <string>
#include <imgui_internal.h>
#include "Panels/asset_browser_panel.h"

// ========================================================================
// 🔥 CONSTRUCTOR & DESTRUCTOR
// Sınıf belleğe çıktığı an panel pointer'ını güvenli bir şekilde oluşturuyoruz.
// Böylece init() çağrılmasa bile pointer asla null kalıp motoru çökertemez.
// ========================================================================
Editor::Editor()
{
    // 🎯 Başlangıçta null veriyoruz çünkü elimizde aria_system yok.
    assetBrowserPanel = nullptr;
}


Editor::~Editor()
{
    if (assetBrowserPanel)
    {
        delete assetBrowserPanel;
        assetBrowserPanel = nullptr;
    }
}

void Editor::init(GLFWwindow*)
{
    // Boş kalabilir, renderUI içinde ilk karede hallediyoruz.
}

void Editor::update(scene&) {}

// =========================
// MAIN UI
// =========================
void Editor::renderUI(scene& scene, EngineContext& ctx, EditorContext& editorCtx)
{
    if (assetBrowserPanel == nullptr && ctx.aria_system != nullptr)
    {
        assetBrowserPanel = new aria::editor::asset_browser_panel(ctx.aria_system);
        assetBrowserPanel->init();
    }

    auto& reg = scene.get_registry();

    editorCtx.focusedPanel = FocusedPanel::None;

    // =========================
    // TOP BAR
    // =========================
    topBarPanel.render(mode, scene, ctx);

    // =========================
    // HIERARCHY
    // =========================
    hierarchyPanel.render(
        scene,
        selectedEntity,
        renamingEntity,
        renameBuffer,
        256,
        deleteQueue,
        editorCtx);

    // =========================
    // SCENE VIEWPORT
    // =========================
    sceneViewportPanel.render(
        sceneTexture,
        sceneViewportSize,
        sceneViewportHovered,
        editorCamera,
        editorCtx);

    // =========================
    // GAME VIEWPORT
    // =========================
    gameViewportPanel.render(
        gameTexture,
        gameViewportSize,
        gameViewportHovered,
        editorCtx);


    if (assetBrowserPanel != nullptr)
    {
        editorCtx.selectedAssetPath = assetBrowserPanel->get_selected_path();
    }
    else
    {
        editorCtx.selectedAssetPath = "";
    }

    // =========================
    // INSPECTOR
    // =========================
    inspectorPanel.render(
        scene,
        selectedEntity,
        ctx,
        editorCtx);

    // =========================
    // 🔥 ASSET BROWSER PANEL
    // =========================
    // GÜVENLİK KONTROLÜ: Pointer'ın geçerli olduğundan emin olup öyle çizdiriyoruz
    if (assetBrowserPanel != nullptr)
    {
        assetBrowserPanel->render(editorCtx);
    }

    // =========================
    // DELETE QUEUE
    // =========================
    if (!deleteQueue.empty())
    {
        // Sahne içindeki sıralı entity listesine erişiyoruz
        auto& sceneEntities = const_cast<std::vector<entity>&>(scene.get_entities());

        for (entity e : deleteQueue)
        {
            if (selectedEntity == (int)e)
                selectedEntity = -1;

            if (renamingEntity == (int)e)
                renamingEntity = -1;

            // 1. Registry ve komponent havuzlarından temizle
            reg.remove_entity(e);

            // 2. Sahne sıralama listesinden (std::vector) bu ID'yi tamamen kaldır
            sceneEntities.erase(
                std::remove(sceneEntities.begin(), sceneEntities.end(), e),
                sceneEntities.end()
            );
        }

        deleteQueue.clear();
    }
}