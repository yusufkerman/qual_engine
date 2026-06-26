#include "game_viewport_panel.h"

#include <imgui.h>


void game_viewport_panel::render(
    unsigned int gameTexture,
    glm::vec2& gameViewportSize,
    bool& gameViewportHovered,
    EditorContext& ctx)
{
    ImGui::Begin("Game");

    // Eğer kullanıcı şu an bu panele tıkladıysa veya bu panel aktifse bağlamı güncelle
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        ctx.focusedPanel = FocusedPanel::GameViewport;
    }

    gameViewportHovered =
        ImGui::IsWindowHovered(
            ImGuiHoveredFlags_RootAndChildWindows);

    ImGui::PushID("GameViewport");

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(0, 0));

    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowBorderSize,
        0.0f);

    ImVec2 avail =
        ImGui::GetContentRegionAvail();

    if (avail.x > 1.0f &&
        avail.y > 1.0f)
    {
        gameViewportSize =
        {
            avail.x,
            avail.y
        };
    }

    if (avail.x > 0 &&
        avail.y > 0 &&
        gameTexture != 0)
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ctx.gameViewportPos = { pos.x, pos.y };
        

        ImGui::Image(
            (ImTextureID)(intptr_t)gameTexture,
            avail,
            ImVec2(0, 1),
            ImVec2(1, 0));
    }

    ImGui::PopStyleVar(2);
    ImGui::PopID();

    ImGui::End();
}