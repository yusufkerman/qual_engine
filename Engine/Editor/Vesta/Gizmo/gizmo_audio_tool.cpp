/**
 * @file gizmo_audio_tool.cpp
 * @brief Sahne içi etkileşimli ses kaynağı min/max etki alanı (Range) tutamaçları aracı uygulaması.
 */

#include "gizmo_audio_tool.h"
#include "gizmo_math_utils.h"
#include "../../../Engine/engine_context.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace vesta
{
    void gizmo_audio_tool::reset_interaction()
    {
        m_ActiveHandle = -1;
        m_HoveredHandle = -1;
    }

    void gizmo_audio_tool::render(
        EngineContext& engineCtx,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& camPos,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        auto& reg = engineCtx.session.editor_scene().get_registry();
        const int selectedID = engineCtx.vestaSys.selection().get_first_selected_entity();
        if (selectedID == -1)
        {
            reset_interaction();
            return;
        }

        const entity e = static_cast<entity>(selectedID);
        auto* src = reg.get_component<audio_source_component>(e);
        const auto* transform = reg.get_component<transform_component>(e);
        if (!src || !transform || !src->is_3d)
        {
            reset_interaction();
            return;
        }

        const glm::mat4 viewProj = proj * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);
        const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
        const glm::vec3 worldPos = glm::vec3(worldMat[3]);

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (!mouseDown)
        {
            m_ActiveHandle = -1;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const gizmo_math::Ray3D mouseRay = gizmo_math::screen_to_world_ray(
            mousePos, invViewProj, viewportPos, viewportSize);

        struct AudioHandlePoint
        {
            int id;
            glm::vec3 worldPt;
            glm::vec3 axisDir;
            bool isMaxDistance; // false: min_distance, true: max_distance
        };

        std::vector<AudioHandlePoint> handles;

        const float minR = std::max(0.1f, src->min_distance);
        const float maxR = std::max(minR + 0.1f, src->max_distance);

        const glm::vec3 dirs[6] = {
            glm::vec3( 1.0f,  0.0f,  0.0f),
            glm::vec3(-1.0f,  0.0f,  0.0f),
            glm::vec3( 0.0f,  1.0f,  0.0f),
            glm::vec3( 0.0f, -1.0f,  0.0f),
            glm::vec3( 0.0f,  0.0f,  1.0f),
            glm::vec3( 0.0f,  0.0f, -1.0f)
        };

        // Handles 0..5: Min distance points
        for (int i = 0; i < 6; ++i)
        {
            handles.push_back({ i, worldPos + dirs[i] * minR, dirs[i], false });
        }
        // Handles 6..11: Max distance points
        for (int i = 0; i < 6; ++i)
        {
            handles.push_back({ 6 + i, worldPos + dirs[i] * maxR, dirs[i], true });
        }

        // Hover tespiti
        if (m_ActiveHandle == -1)
        {
            m_HoveredHandle = -1;
            float bestDist = 11.0f;
            for (const auto& h : handles)
            {
                ImVec2 sp;
                if (gizmo_math::project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                {
                    const float d = std::hypot(mousePos.x - sp.x, mousePos.y - sp.y);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        m_HoveredHandle = h.id;
                    }
                }
            }

            if (m_HoveredHandle != -1 && mouseClicked)
            {
                m_ActiveHandle = m_HoveredHandle;
                m_AudioDragStartMinDist = src->min_distance;
                m_AudioDragStartMaxDist = src->max_distance;
            }
        }

        // Aktif sürükleme güncellemesi
        if (m_ActiveHandle != -1 && mouseDown)
        {
            for (const auto& h : handles)
            {
                if (h.id != m_ActiveHandle) continue;

                const float param = gizmo_math::ray_axis_closest_param(mouseRay, worldPos, h.axisDir);
                const float distVal = std::max(0.1f, std::abs(param));

                if (!h.isMaxDistance)
                {
                    src->min_distance = std::clamp(distVal, 0.1f, src->max_distance - 0.05f);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Audio Min Distance: %.2f m", src->min_distance);
                    ImGui::SetTooltip("%s", buf);
                }
                else
                {
                    src->max_distance = std::max(src->min_distance + 0.05f, distVal);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Audio Max Distance: %.2f m", src->max_distance);
                    ImGui::SetTooltip("%s", buf);
                }
                break;
            }
        }

        // Tutamaç çizimi
        for (const auto& h : handles)
        {
            ImVec2 sp;
            if (!gizmo_math::project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                continue;

            const bool isHot = (m_HoveredHandle == h.id) || (m_ActiveHandle == h.id);
            const float halfSz = isHot ? 5.5f : 3.8f;

            const ImU32 fillCol = isHot
                ? IM_COL32(255, 255, 255, 255)
                : (h.isMaxDistance ? IM_COL32(255, 165, 45, 240) : IM_COL32(50, 235, 165, 240));
            const ImU32 borderCol = IM_COL32(20, 22, 28, 240);

            dl->AddRectFilled(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), fillCol, 1.0f);
            dl->AddRect(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), borderCol, 1.0f, 0, 1.4f);
        }
    }
} // namespace vesta

