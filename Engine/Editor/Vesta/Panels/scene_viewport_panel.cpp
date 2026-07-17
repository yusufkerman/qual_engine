#include "scene_viewport_panel.h"
#include <imgui.h>
#include <iostream>

namespace vesta {

    void scene_viewport_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);

        m_viewport_state.scene().focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        m_viewport_state.scene().hovered = ImGui::IsWindowHovered(ImGuiFocusedFlags_RootAndChildWindows);


        ImGui::PushID("SceneViewport");
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 1.0f && avail.y > 1.0f)
        {
            m_viewport_state.scene().size = { avail.x, avail.y };
        }

        unsigned int tex = m_vm.get_texture_id();
        if (avail.x > 0 && avail.y > 0 && tex != 0)
        {
            ImVec2 cursor = ImGui::GetCursorScreenPos();

            ImGui::GetWindowDrawList()->PushClipRect(
                cursor,
                ImVec2(cursor.x + avail.x, cursor.y + avail.y),
                true
            );

            ImGui::Image((ImTextureID)(intptr_t)tex, avail, ImVec2(0, 1), ImVec2(1, 0));

            ImGui::GetWindowDrawList()->PopClipRect();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopID();
        ImGui::End();
    }

} // namespace vesta