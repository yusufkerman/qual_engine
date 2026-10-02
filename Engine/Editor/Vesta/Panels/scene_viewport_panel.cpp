/**
 * @file scene_viewport_panel.cpp
 * @brief Sahne görünümü (Scene Viewport) panelinin ImGui ile çizim işlemlerini barındırır.
 */

#include "scene_viewport_panel.h"
#include <imgui.h>

namespace vesta {

    scene_viewport_panel::scene_viewport_panel(
        scene_viewport_view_model& vm,
        editor_viewport_state& viewportState)
        : m_vm(vm)
        , m_viewport_state(viewportState)
    {
    }

    void scene_viewport_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);

        auto& scene_state = m_viewport_state.scene();

        // Girdi (Input), kısayol ve fly-cam kontrolleri için odak/imleç durumlarını güncelle
        scene_state.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        scene_state.hovered = ImGui::IsWindowHovered(ImGuiFocusedFlags_RootAndChildWindows);

        ImGui::PushID("SceneViewport");

        // Render alanının pencereye tam oturması için kenar boşluklarını sıfırla
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 1.0f && avail.y > 1.0f)
        {
            scene_state.size = { avail.x, avail.y };
        }

        unsigned int tex = m_vm.get_texture_id();
        if (avail.x > 0.0f && avail.y > 0.0f && tex != 0)
        {
            ImVec2 cursor = ImGui::GetCursorScreenPos();

            // Gizmo ve raycast/picking işlemleri için sol üst ekran koordinatını kaydet
            scene_state.position = { cursor.x, cursor.y };

            // Çizimin panel sınırları dışına taşmasını engellemek için kırpma (clipping) uygula
            ImGui::GetWindowDrawList()->PushClipRect(
                cursor,
                ImVec2(cursor.x + avail.x, cursor.y + avail.y),
                true
            );

            // OpenGL UV koordinatlarını (0,1 -> 1,0) ters çevirerek FBO dokusunu bas
            ImGui::Image(
                (ImTextureID)(intptr_t)tex,
                avail,
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            ImGui::GetWindowDrawList()->PopClipRect();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopID();
        ImGui::End();
    }

} // namespace vesta