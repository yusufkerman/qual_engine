#include "editor.h"
#include "../Scene/scene.h"

#include <imgui.h>
#include <string>
#include <imgui_internal.h>

void Editor::init(GLFWwindow*) {}

void Editor::update(scene&) {}

// =========================
// MAIN UI
// =========================
void Editor::renderUI(scene& scene)
{
    auto& reg = scene.get_registry();

    // =========================
    // TOP BAR
    // =========================
    topBarPanel.render(mode);

    // =========================
    // HIERARCHY
    // =========================
    hierarchyPanel.render(
        scene,
        selectedEntity,
        renamingEntity,
        renameBuffer,
        256,
        deleteQueue);

    // =========================
    // SCENE VIEWPORT
    // =========================
    sceneViewportPanel.render(
        sceneTexture,
        sceneViewportSize,
        sceneViewportHovered,
        editorCamera);

    // =========================
    // GAME VIEWPORT
    // =========================
    gameViewportPanel.render(
        gameTexture,
        gameViewportSize,
        gameViewportHovered);

    // =========================
    // INSPECTOR
    // =========================
    inspectorPanel.render(
        scene,
        selectedEntity);

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