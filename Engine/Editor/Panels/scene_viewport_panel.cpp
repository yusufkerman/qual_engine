#include "scene_viewport_panel.h"

#include <imgui.h>

void scene_viewport_panel::render(
    unsigned int sceneTexture,
    glm::vec2& sceneViewportSize,
    bool& sceneViewportHovered,
    EditorCamera& editorCamera)
{
    ImGui::Begin("Scene");

    sceneViewportHovered =
        ImGui::IsWindowHovered(
            ImGuiHoveredFlags_RootAndChildWindows);

    editorCamera.sceneFocused =
        ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows);

    editorCamera.sceneHovered =
        sceneViewportHovered;

    ImGui::PushID("SceneViewport");

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(0, 0));

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowBorderSize,
        0.0f);

    ImVec2 avail =
        ImGui::GetContentRegionAvail();

    if (avail.x > 1.0f && avail.y > 1.0f)
    {
        sceneViewportSize =
        {
            avail.x,
            avail.y
        };
    }

    if (avail.x > 0 &&
        avail.y > 0 &&
        sceneTexture != 0)
    {
        ImVec2 cursor =
            ImGui::GetCursorScreenPos();

        ImGui::GetWindowDrawList()->PushClipRect(
            cursor,
            ImVec2(
                cursor.x + avail.x,
                cursor.y + avail.y),
            true);

        ImGui::Image(
            (ImTextureID)(intptr_t)sceneTexture,
            avail,
            ImVec2(0, 1),
            ImVec2(1, 0));

        ImGui::GetWindowDrawList()->PopClipRect();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopID();

    ImGui::End();
}