#include "game_viewport_panel.h"
#include <imgui.h>

namespace vesta {

    void game_viewport_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);

        auto& state = m_viewport_state.game();

        state.focused =
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        state.hovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        ImGui::PushID("GameViewport");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImVec2 avail = ImGui::GetContentRegionAvail();

        if (avail.x > 1.0f && avail.y > 1.0f)
        {
            state.size = { avail.x, avail.y };
        }

        unsigned int tex = m_vm.get_texture_id();

        if (tex != 0 && avail.x > 0.0f && avail.y > 0.0f)
        {
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            state.position = { screenPos.x, screenPos.y };

            ImGui::Image(
                (ImTextureID)(intptr_t)tex,
                avail,
                ImVec2(0, 1),
                ImVec2(1, 0));
        }

        ImGui::PopStyleVar(2);
        ImGui::PopID();
        ImGui::End();
    }

} // namespace vesta