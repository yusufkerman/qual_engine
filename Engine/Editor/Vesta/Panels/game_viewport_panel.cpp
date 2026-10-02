/**
 * @file game_viewport_panel.cpp
 * @brief Oyun görünümü (Game Viewport) panelinin ImGui ile çizim işlemlerini barındırır.
 */

#include "game_viewport_panel.h"
#include <imgui.h>

namespace vesta {

    game_viewport_panel::game_viewport_panel(
        game_viewport_view_model& vm,
        editor_viewport_state& viewportState)
        : m_vm(vm)
        , m_viewport_state(viewportState)
    {
    }

    void game_viewport_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);

        // Çekirdek editör durumundan oyun ekranı verisine ulaş
        auto& state = m_viewport_state.game();

        // Girdi (Input) ve kısayolların doğru çalışması için odak/imleç durumlarını güncelle
        state.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        state.hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        ImGui::PushID("GameViewport");

        // Render hedefinin pencere kenar boşlukları olmadan alana tam sığması için padding'i sıfırla
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

            // OpenGL dokularının Y ekseni ters olduğu için UV koordinatlarını (0,1 -> 1,0) ters çevirerek çiz
            ImGui::Image(
                (ImTextureID)(intptr_t)tex,
                avail,
                ImVec2(0, 1),
                ImVec2(1, 0)
            );
        }

        ImGui::PopStyleVar(2);
        ImGui::PopID();
        ImGui::End();
    }

    const std::string& game_viewport_panel::get_title() const
    {
        return m_title;
    }

    bool game_viewport_panel::is_open() const
    {
        return m_is_open;
    }

    void game_viewport_panel::set_open(bool open)
    {
        m_is_open = open;
    }

} // namespace vesta