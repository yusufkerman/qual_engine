#include "game_viewport_panel.h"
#include <imgui.h>

namespace vesta {

    void game_viewport_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);

        // Odak ve Hover durumlarını doğrudan View Model'e yazıyoruz
        m_viewport_state.game().focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        m_viewport_state.game().hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        ImGui::PushID("GameViewport");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 1.0f && avail.y > 1.0f)
        {
            m_viewport_state.game().size = { avail.x, avail.y };
        }

        unsigned int tex = m_vm.get_texture_id();
        if (avail.x > 0 && avail.y > 0 && tex != 0)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            m_viewport_state.game().position = { pos.x, pos.y };

            ImGui::Image((ImTextureID)(intptr_t)tex, avail, ImVec2(0, 1), ImVec2(1, 0));
        }


        ImGui::PopStyleVar(2);
        ImGui::PopID();
        ImGui::End();
    }

} // namespace vesta