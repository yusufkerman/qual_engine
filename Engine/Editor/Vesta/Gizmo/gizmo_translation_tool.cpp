/**
 * @file gizmo_translation_tool.cpp
 * @brief Sahne içi 3B konum taşıma (Translate) gizmo aracı uygulaması.
 */

#include "gizmo_translation_tool.h"
#include "../../../Engine/engine_context.h"
#include "../../../Renderer/Debug/gizmo_settings.h"

namespace vesta
{
    void gizmo_translation_tool::reset_interaction()
    {
        m_Active = Handle::None;
        m_Hovered = Handle::None;
        m_DragState.clear();
    }

    void gizmo_translation_tool::process(const gizmo_tool_context& ctx)
    {
        auto& settings = ctx.engineCtx.gizmoSystem.settings();
        auto& reg = ctx.engineCtx.session.editor_scene().get_registry();
        ImDrawList* dl = ctx.drawList;

        if (!ctx.isMouseDown)
        {
            m_Active = Handle::None;
            m_DragState.clear();
        }

        const glm::vec3 axes[3] = {
            glm::normalize(ctx.basis[0]),
            glm::normalize(ctx.basis[1]),
            glm::normalize(ctx.basis[2])
        };

        const glm::vec3 tipWorld[3] = {
            ctx.originWorld + axes[0] * ctx.gizmoWorldScale,
            ctx.originWorld + axes[1] * ctx.gizmoWorldScale,
            ctx.originWorld + axes[2] * ctx.gizmoWorldScale
        };

        ImVec2 tipScreen[3];
        bool tipValid[3];
        for (int i = 0; i < 3; ++i)
        {
            tipValid[i] = gizmo_math::project_world_to_screen(
                tipWorld[i], ctx.viewProj, ctx.viewportPos, ctx.viewportSize, tipScreen[i]);
        }

        const gizmo_math::Ray3D mouseRay = gizmo_math::screen_to_world_ray(
            ctx.mousePos, ctx.invViewProj, ctx.viewportPos, ctx.viewportSize);

        // 1. Hover Tespiti ve Tıklama
        if (m_Active == Handle::None)
        {
            m_Hovered = Handle::None;
            float bestDist = 10.0f;

            // Merkez serbest taşıma dairesi
            const float centerDist = std::hypot(ctx.mousePos.x - ctx.originScreen.x, ctx.mousePos.y - ctx.originScreen.y);
            if (centerDist <= 9.5f)
            {
                m_Hovered = Handle::Center;
                bestDist = centerDist;
            }

            // İki eksenli düzlem kutucukları (PlaneXY, PlaneXZ, PlaneYZ)
            const int planePairs[3][2] = { {0, 1}, {0, 2}, {1, 2} };
            const Handle planeIDs[3] = { Handle::PlaneXY, Handle::PlaneXZ, Handle::PlaneYZ };
            for (int p = 0; p < 3 && m_Hovered == Handle::None; ++p)
            {
                const glm::vec3 pCenter = ctx.originWorld + (axes[planePairs[p][0]] + axes[planePairs[p][1]]) * (ctx.gizmoWorldScale * 0.32f);
                ImVec2 pScreen;
                if (gizmo_math::project_world_to_screen(pCenter, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, pScreen))
                {
                    if (std::hypot(ctx.mousePos.x - pScreen.x, ctx.mousePos.y - pScreen.y) <= 10.0f)
                    {
                        m_Hovered = planeIDs[p];
                    }
                }
            }

            // Tekil eksen okları (AxisX, AxisY, AxisZ)
            if (m_Hovered == Handle::None)
            {
                for (int i = 0; i < 3; ++i)
                {
                    if (!tipValid[i]) continue;
                    const float d = gizmo_math::dist_point_to_segment_2d(ctx.mousePos, ctx.originScreen, tipScreen[i]);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        m_Hovered = static_cast<Handle>(static_cast<int>(Handle::AxisX) + i);
                    }
                }
            }

            // Tıklama ile sürüklemeyi başlat
            if (m_Hovered != Handle::None && ctx.isMouseClicked)
            {
                m_Active = m_Hovered;
                m_DragState.capture(ctx.activeEntities, ctx.primaryEntity, ctx.originWorld, ctx.mousePos, reg);

                if (m_Active == Handle::AxisX || m_Active == Handle::AxisY || m_Active == Handle::AxisZ)
                {
                    const int idx = static_cast<int>(m_Active) - static_cast<int>(Handle::AxisX);
                    m_DragAxis0 = axes[idx];
                    m_DragStartAxisParam = gizmo_math::ray_axis_closest_param(mouseRay, ctx.originWorld, m_DragAxis0);
                }
                else
                {
                    if (m_Active == Handle::PlaneXY)      m_DragPlaneNormal = axes[2];
                    else if (m_Active == Handle::PlaneXZ) m_DragPlaneNormal = axes[1];
                    else if (m_Active == Handle::PlaneYZ) m_DragPlaneNormal = axes[0];
                    else                                  m_DragPlaneNormal = glm::normalize(ctx.camPos - ctx.originWorld);

                    if (!gizmo_math::ray_plane_intersect(mouseRay, ctx.originWorld, m_DragPlaneNormal, m_DragStartHitPoint))
                    {
                        m_DragStartHitPoint = ctx.originWorld;
                    }
                }
            }
        }

        // 2. Aktif Sürükleme ve Pozisyon Güncellemesi
        if (m_Active != Handle::None && ctx.isMouseDown && !m_DragState.empty())
        {
            glm::vec3 newWorldPos = m_DragState.startWorldPos;

            if (m_Active == Handle::AxisX || m_Active == Handle::AxisY || m_Active == Handle::AxisZ)
            {
                const float curParam = gizmo_math::ray_axis_closest_param(mouseRay, m_DragState.startWorldPos, m_DragAxis0);
                float delta = curParam - m_DragStartAxisParam;
                if (ctx.snapActive) {
                    delta = gizmo_math::snap_value(delta, settings.translate_snap);
                }
                newWorldPos = m_DragState.startWorldPos + m_DragAxis0 * delta;

                // Sonsuz eksen kılavuz çizgisi çiz
                ImVec2 g0, g1;
                if (gizmo_math::project_world_to_screen(m_DragState.startWorldPos - m_DragAxis0 * 50.0f, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, g0) &&
                    gizmo_math::project_world_to_screen(m_DragState.startWorldPos + m_DragAxis0 * 50.0f, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, g1))
                {
                    dl->AddLine(g0, g1, IM_COL32(255, 240, 120, 110), 1.5f);
                }
            }
            else
            {
                glm::vec3 hitPt;
                if (gizmo_math::ray_plane_intersect(mouseRay, m_DragState.startWorldPos, m_DragPlaneNormal, hitPt))
                {
                    glm::vec3 deltaVec = hitPt - m_DragStartHitPoint;
                    if (ctx.snapActive)
                    {
                        deltaVec.x = gizmo_math::snap_value(deltaVec.x, settings.translate_snap);
                        deltaVec.y = gizmo_math::snap_value(deltaVec.y, settings.translate_snap);
                        deltaVec.z = gizmo_math::snap_value(deltaVec.z, settings.translate_snap);
                    }
                    newWorldPos = m_DragState.startWorldPos + deltaVec;
                }
            }

            const glm::vec3 deltaWorld = newWorldPos - m_DragState.startWorldPos;

            for (size_t i = 0; i < m_DragState.dragEntities.size(); ++i)
            {
                const entity ent = m_DragState.dragEntities[i];
                const glm::vec3 targetWorld = m_DragState.startWorldPositions[i] + deltaWorld;
                gizmo_math::apply_entity_world_position(ent, targetWorld, reg);
            }

            if (m_DragState.dragEntities.size() > 1)
            {
                ImGui::SetTooltip("Moving %zu Entities\nDelta: (%.2f, %.2f, %.2f)",
                    m_DragState.dragEntities.size(), deltaWorld.x, deltaWorld.y, deltaWorld.z);
            }
            else
            {
                const auto* primaryTransform = reg.get_component<transform_component>(ctx.primaryEntity);
                if (primaryTransform)
                {
                    ImGui::SetTooltip("Pos: (%.2f, %.2f, %.2f)",
                        primaryTransform->position.x, primaryTransform->position.y, primaryTransform->position.z);
                }
            }
        }

        // 3. Çizim: Düzlem Kutucukları (Plane Quads)
        const int planePairs[3][2] = { {0, 1}, {0, 2}, {1, 2} };
        const Handle planeIDs[3] = { Handle::PlaneXY, Handle::PlaneXZ, Handle::PlaneYZ };

        for (int p = 0; p < 3; ++p)
        {
            const glm::vec3 a0 = axes[planePairs[p][0]];
            const glm::vec3 a1 = axes[planePairs[p][1]];
            const float s0 = ctx.gizmoWorldScale * 0.20f;
            const float s1 = ctx.gizmoWorldScale * 0.42f;

            ImVec2 q[4];
            if (gizmo_math::project_world_to_screen(ctx.originWorld + a0 * s0 + a1 * s0, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, q[0]) &&
                gizmo_math::project_world_to_screen(ctx.originWorld + a0 * s1 + a1 * s0, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, q[1]) &&
                gizmo_math::project_world_to_screen(ctx.originWorld + a0 * s1 + a1 * s1, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, q[2]) &&
                gizmo_math::project_world_to_screen(ctx.originWorld + a0 * s0 + a1 * s1, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, q[3]))
            {
                const bool hot = (m_Hovered == planeIDs[p]) || (m_Active == planeIDs[p]);
                dl->AddQuadFilled(q[0], q[1], q[2], q[3], hot ? IM_COL32(255, 232, 68, 140) : gizmo_math::kPlaneFillColors[p]);
                dl->AddQuad(q[0], q[1], q[2], q[3], hot ? gizmo_math::kColorHover : gizmo_math::kPlaneBorderColors[p], 1.6f);
            }
        }

        // 4. Çizim: X, Y, Z Eksen Okları
        for (int i = 0; i < 3; ++i)
        {
            if (!tipValid[i]) continue;
            const Handle id = static_cast<Handle>(static_cast<int>(Handle::AxisX) + i);
            const bool hot = (m_Hovered == id) || (m_Active == id);
            const ImU32 col = hot ? gizmo_math::kColorHover : gizmo_math::kAxisColors[i];

            dl->AddLine(ctx.originScreen, tipScreen[i], col, hot ? 3.8f : 2.8f);

            // Ok başı üçgeni
            const float dx = tipScreen[i].x - ctx.originScreen.x;
            const float dy = tipScreen[i].y - ctx.originScreen.y;
            const float len = std::hypot(dx, dy);
            if (len > 8.0f)
            {
                const float ux = dx / len;
                const float uy = dy / len;
                const float px = -uy;
                const float py = ux;

                const ImVec2 headTip(tipScreen[i].x + ux * 6.0f, tipScreen[i].y + uy * 6.0f);
                const ImVec2 baseL(tipScreen[i].x - ux * 8.0f + px * 5.5f, tipScreen[i].y - uy * 8.0f + py * 5.5f);
                const ImVec2 baseR(tipScreen[i].x - ux * 8.0f - px * 5.5f, tipScreen[i].y - uy * 8.0f - py * 5.5f);
                dl->AddTriangleFilled(headTip, baseL, baseR, col);
            }
        }

        // 5. Çizim: Merkez Serbest Taşıma Noktası
        const bool centerHot = (m_Hovered == Handle::Center) || (m_Active == Handle::Center);
        dl->AddCircleFilled(ctx.originScreen, centerHot ? 6.5f : 5.0f, centerHot ? gizmo_math::kColorHover : IM_COL32(245, 245, 250, 240), 16);
        dl->AddCircle(ctx.originScreen, centerHot ? 6.5f : 5.0f, IM_COL32(20, 22, 28, 230), 16, 1.5f);
    }
} // namespace vesta

