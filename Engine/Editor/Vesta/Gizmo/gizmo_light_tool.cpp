/**
 * @file gizmo_light_tool.cpp
 * @brief Sahne içi etkileşimli ışık menzili ve spot koni açısı tutamaçları aracı uygulaması.
 */

#include "gizmo_light_tool.h"
#include "gizmo_math_utils.h"
#include "../../../Engine/engine_context.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace vesta
{
    void gizmo_light_tool::reset_interaction()
    {
        m_ActiveHandle = -1;
        m_HoveredHandle = -1;
    }

    void gizmo_light_tool::render(
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
        auto* light = reg.get_component<light_component>(e);
        const auto* transform = reg.get_component<transform_component>(e);
        if (!light || !transform || light->type == light_type::directional)
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

        struct LightHandlePoint
        {
            int id;
            glm::vec3 worldPt;
            glm::vec3 axisDir;
            bool isInnerCone;
        };

        std::vector<LightHandlePoint> handles;

        // Işık yönü ve ortonormal tabanı
        const glm::mat3 rotMat = gizmo_math::euler_to_mat3(transform->rotation);
        const glm::vec3 forward = glm::normalize(rotMat * glm::vec3(0.0f, 0.0f, -1.0f));
        glm::vec3 right, up;
        gizmo_math::build_basis(forward, right, up);

        if (light->type == light_type::point)
        {
            const float r = std::max(0.1f, light->range);
            const glm::vec3 dirs[6] = {
                glm::vec3( 1.0f,  0.0f,  0.0f),
                glm::vec3(-1.0f,  0.0f,  0.0f),
                glm::vec3( 0.0f,  1.0f,  0.0f),
                glm::vec3( 0.0f, -1.0f,  0.0f),
                glm::vec3( 0.0f,  0.0f,  1.0f),
                glm::vec3( 0.0f,  0.0f, -1.0f)
            };
            for (int i = 0; i < 6; ++i)
            {
                handles.push_back({ i, worldPos + dirs[i] * r, dirs[i], false });
            }
        }
        else if (light->type == light_type::spot)
        {
            const float r = std::max(0.1f, light->range);
            const float outerDeg = std::clamp(light->outer_cone, 1.0f, 85.0f);
            const float innerDeg = std::clamp(light->inner_cone, 0.5f, outerDeg);

            const float outerRad = glm::radians(outerDeg);
            const float innerRad = glm::radians(innerDeg);

            const glm::vec3 outerCenter = worldPos + forward * (r * std::cos(outerRad));
            const float outerRadius = r * std::sin(outerRad);

            const glm::vec3 innerCenter = worldPos + forward * (r * std::cos(innerRad));
            const float innerRadius = r * std::sin(innerRad);

            // Handle 0: Menzil ucu (Range)
            handles.push_back({ 0, worldPos + forward * r, forward, false });

            // Handles 1..4: Dış koni çemberi (Outer Cone Angle)
            handles.push_back({ 1, outerCenter + right * outerRadius,  right, false });
            handles.push_back({ 2, outerCenter - right * outerRadius, -right, false });
            handles.push_back({ 3, outerCenter + up * outerRadius,     up,    false });
            handles.push_back({ 4, outerCenter - up * outerRadius,    -up,    false });

            // Handles 5..8: İç koni çemberi (Inner Cone Angle)
            handles.push_back({ 5, innerCenter + right * innerRadius,  right, true });
            handles.push_back({ 6, innerCenter - right * innerRadius, -right, true });
            handles.push_back({ 7, innerCenter + up * innerRadius,     up,    true });
            handles.push_back({ 8, innerCenter - up * innerRadius,    -up,    true });
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
                m_LightDragStartRange = light->range;
                m_LightDragStartOuterCone = light->outer_cone;
                m_LightDragStartInnerCone = light->inner_cone;
            }
        }

        // Aktif sürükleme güncellemesi
        if (m_ActiveHandle != -1 && mouseDown)
        {
            if (light->type == light_type::point)
            {
                const glm::vec3 axisDir = handles[std::clamp(m_ActiveHandle, 0, 5)].axisDir;
                const float param = gizmo_math::ray_axis_closest_param(mouseRay, worldPos, axisDir);
                light->range = std::max(0.1f, std::abs(param));
                ImGui::SetTooltip("Point Light Range: %.2f m", light->range);
            }
            else if (light->type == light_type::spot)
            {
                if (m_ActiveHandle == 0) // Menzil tutamacı
                {
                    const float param = gizmo_math::ray_axis_closest_param(mouseRay, worldPos, forward);
                    light->range = std::max(0.2f, param);
                    ImGui::SetTooltip("Spot Light Range: %.2f m", light->range);
                }
                else if (m_ActiveHandle >= 1 && m_ActiveHandle <= 4) // Dış koni tutamacı
                {
                    const glm::vec3 sideDir = handles[m_ActiveHandle].axisDir;
                    const glm::vec3 planeOrigin = worldPos + forward * (light->range * std::cos(glm::radians(light->outer_cone)));
                    glm::vec3 hitPt;
                    if (gizmo_math::ray_plane_intersect(mouseRay, planeOrigin, forward, hitPt))
                    {
                        const float sideDist = std::max(0.01f, glm::dot(hitPt - planeOrigin, sideDir));
                        const float angleRad = std::atan2(sideDist, light->range);
                        light->outer_cone = std::clamp(glm::degrees(angleRad), light->inner_cone + 0.5f, 85.0f);
                        ImGui::SetTooltip("Spot Outer Cone: %.1f deg", light->outer_cone);
                    }
                }
                else if (m_ActiveHandle >= 5 && m_ActiveHandle <= 8) // İç koni tutamacı
                {
                    const glm::vec3 sideDir = handles[m_ActiveHandle].axisDir;
                    const glm::vec3 planeOrigin = worldPos + forward * (light->range * std::cos(glm::radians(light->inner_cone)));
                    glm::vec3 hitPt;
                    if (gizmo_math::ray_plane_intersect(mouseRay, planeOrigin, forward, hitPt))
                    {
                        const float sideDist = std::max(0.01f, glm::dot(hitPt - planeOrigin, sideDir));
                        const float angleRad = std::atan2(sideDist, light->range);
                        light->inner_cone = std::clamp(glm::degrees(angleRad), 0.5f, light->outer_cone - 0.5f);
                        ImGui::SetTooltip("Spot Inner Cone: %.1f deg", light->inner_cone);
                    }
                }
            }
        }

        // Tutamaçların ekranda çizimi
        for (const auto& h : handles)
        {
            ImVec2 sp;
            if (!gizmo_math::project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                continue;

            const bool isHot = (m_HoveredHandle == h.id) || (m_ActiveHandle == h.id);
            const float halfSz = isHot ? 5.5f : 3.8f;
            const ImU32 fillCol = isHot ? IM_COL32(255, 255, 255, 255)
                                        : (h.isInnerCone ? IM_COL32(255, 210, 60, 240) : IM_COL32(255, 140, 40, 240));
            const ImU32 borderCol = IM_COL32(20, 22, 28, 240);

            dl->AddRectFilled(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), fillCol, 1.0f);
            dl->AddRect(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), borderCol, 1.0f, 0, 1.4f);
        }
    }
} // namespace vesta

