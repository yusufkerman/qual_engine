#include "editor.h"
#include "../Scene/scene.h"
#include "../Engine/engine_context.h"

#include <imgui.h>
#include <string>
#include <imgui_internal.h>

void Editor::init(GLFWwindow*) {}

void Editor::update(scene&) {}

// =========================
// MAIN UI
// =========================
void Editor::renderUI(scene& scene, EngineContext& ctx, EditorContext& editorCtx)
{
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

    // =========================
    // INSPECTOR
    // =========================
    inspectorPanel.render(
        scene,
        selectedEntity,
        ctx,
        editorCtx);

    // =========================
    // DELETE QUEUE
    // =========================
    for (entity e : deleteQueue)
    {
        if (selectedEntity == (int)e)
            selectedEntity = -1;

        if (renamingEntity == (int)e)
            renamingEntity = -1;

        reg.remove_entity(e);
    }

    deleteQueue.clear();
}