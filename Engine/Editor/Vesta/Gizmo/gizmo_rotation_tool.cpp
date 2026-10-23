/**
 * @file gizmo_rotation_tool.cpp
 * @brief Sahne içi 3B yönelim döndürme (Rotate) gizmo aracı uygulaması.
 */

#include "gizmo_rotation_tool.h"
#include "../../../Engine/engine_context.h"
#include "../../../Renderer/Debug/gizmo_settings.h"

namespace vesta
{
    void gizmo_rotation_tool::reset_interaction()
    {
        m_Active = Handle::None;
        m_Hovered = Handle::None;
        m_DragState.clear();
        m_CurrentRotateDeltaDeg = 0.0f;
    }

    void gizmo_rotation_tool::process(const gizmo_tool_context& ctx)
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

        const float ringRadius = ctx.gizmoWorldScale * 0.92f;
        constexpr int ringSegments = 56;
        const glm::vec3 viewDirToCenter = glm::normalize(ctx.originWorld - ctx.camPos);

        // 1. Hover Tespiti ve Tıklama
        if (m_Active == Handle::None)
        {
            m_Hovered = Handle::None;
            float bestDist = 9.0f;

            for (int i = 0; i < 3; ++i)
            {
                glm::vec3 u, v;
                gizmo_math::build_basis(axes[i], u, v);
                const float step = glm::two_pi<float>() / static_cast<float>(ringSegments);

                for (int s = 0; s < ringSegments; ++s)
                {
                    const float a0 = static_cast<float>(s) * step;
                    const float a1 = static_cast<float>(s + 1) * step;

                    const glm::vec3 p0 = ctx.originWorld + (u * std::cos(a0) + v * std::sin(a0)) * ringRadius;
                    const glm::vec3 p1 = ctx.originWorld + (u * std::cos(a1) + v * std::sin(a1)) * ringRadius;

                    ImVec2 sp0, sp1;
                    if (gizmo_math::project_world_to_screen(p0, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, sp0) &&
                        gizmo_math::project_world_to_screen(p1, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, sp1))
                    {
                        const float d = gizmo_math::dist_point_to_segment_2d(ctx.mousePos, sp0, sp1);
                        if (d < bestDist)
                        {
                            bestDist = d;
                            m_Hovered = static_cast<Handle>(static_cast<int>(Handle::AxisX) + i);
                        }
                    }
                }
            }

            if (m_Hovered != Handle::None && ctx.isMouseClicked)
            {
                m_Active = m_Hovered;
                m_DragState.capture(ctx.activeEntities, ctx.primaryEntity, ctx.originWorld, ctx.mousePos, reg);
                m_CurrentRotateDeltaDeg = 0.0f;

                const int idx = static_cast<int>(m_Active) - static_cast<int>(Handle::AxisX);
                m_DragAxis0 = axes[idx];
            }
        }

        // 2. Aktif Sürükleme Hesaplaması
        if (m_Active != Handle::None && ctx.isMouseDown && !m_DragState.empty())
        {
            // Ekran uzayında merkez etrafındaki teğetsel fare hareketinden pürüzsüz açısal delta hesapla
            const glm::vec2 toStart(m_DragState.startMousePos.x - ctx.originScreen.x, m_DragState.startMousePos.y - ctx.originScreen.y);
            glm::vec2 tangent(-toStart.y, toStart.x);
            if (glm::length(tangent) < 1.0f) {
                tangent = glm::vec2(1.0f, 0.0f);
            }
            else {
                tangent = glm::normalize(tangent);
            }

            // Kamera bakış yönüne göre dönüş işaretini düzelt
            const float viewSign = (glm::dot(m_DragAxis0, viewDirToCenter) < 0.0f) ? 1.0f : -1.0f;
            const glm::vec2 mouseDelta(ctx.mousePos.x - m_DragState.startMousePos.x, ctx.mousePos.y - m_DragState.startMousePos.y);
            float angleDeg = glm::dot(mouseDelta, tangent) * 0.65f * viewSign;

            if (ctx.snapActive)
            {
                angleDeg = gizmo_math::snap_value(angleDeg, settings.rotate_snap_deg);
            }

            m_CurrentRotateDeltaDeg = angleDeg;
            const float angleRad = glm::radians(angleDeg);

            const glm::mat4 deltaRotMat4 = glm::rotate(glm::mat4(1.0f), angleRad, m_DragAxis0);
            const glm::mat3 deltaRotMat3 = glm::mat3(deltaRotMat4);

            for (size_t i = 0; i < m_DragState.dragEntities.size(); ++i)
            {
                const entity ent = m_DragState.dragEntities[i];
                auto* t = reg.get_component<transform_component>(ent);
                if (!t) continue;

                // Rotasyon etrafında dönme (Orbiting world position around selection center)
                const glm::vec3 offset = m_DragState.startWorldPositions[i] - m_DragState.startWorldPos;
                const glm::vec3 rotatedOffset = deltaRotMat3 * offset;
                const glm::vec3 targetWorld = m_DragState.startWorldPos + rotatedOffset;

                gizmo_math::apply_entity_world_position(ent, targetWorld, reg);

                // Yerel yönelim rotasyonu
                const glm::mat3 startRotMat = gizmo_math::euler_to_mat3(m_DragState.startRotations[i]);
                const glm::mat3 finalRotMat = deltaRotMat3 * startRotMat;
                t->rotation = gizmo_math::mat3_to_euler(finalRotMat);
            }

            if (m_DragState.dragEntities.size() > 1)
            {
                ImGui::SetTooltip("Rotating %zu Entities\nAngle: %+.1f deg", m_DragState.dragEntities.size(), m_CurrentRotateDeltaDeg);
            }
            else
            {
                const auto* primaryTransform = reg.get_component<transform_component>(ctx.primaryEntity);
                if (primaryTransform)
                {
                    ImGui::SetTooltip("Rotate Delta: %+.1f deg\nEuler (rad): (%.2f, %.2f, %.2f)",
                        m_CurrentRotateDeltaDeg, primaryTransform->rotation.x, primaryTransform->rotation.y, primaryTransform->rotation.z);
                }
            }
        }

        // 3. Çizim: Dış referans çemberi ve X, Y, Z dönüş halkaları
        dl->AddCircle(ctx.originScreen, settings.handle_size * 0.98f, IM_COL32(220, 225, 235, 70), 48, 1.4f);

        for (int i = 0; i < 3; ++i)
        {
            const Handle id = static_cast<Handle>(static_cast<int>(Handle::AxisX) + i);
            const bool hot = (m_Hovered == id) || (m_Active == id);

            glm::vec3 u, v;
            gizmo_math::build_basis(axes[i], u, v);
            const float step = glm::two_pi<float>() / static_cast<float>(ringSegments);

            for (int s = 0; s < ringSegments; ++s)
            {
                const float a0 = static_cast<float>(s) * step;
                const float a1 = static_cast<float>(s + 1) * step;

                const glm::vec3 dir0 = u * std::cos(a0) + v * std::sin(a0);
                const glm::vec3 dir1 = u * std::cos(a1) + v * std::sin(a1);

                const glm::vec3 p0 = ctx.originWorld + dir0 * ringRadius;
                const glm::vec3 p1 = ctx.originWorld + dir1 * ringRadius;

                ImVec2 sp0, sp1;
                if (gizmo_math::project_world_to_screen(p0, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, sp0) &&
                    gizmo_math::project_world_to_screen(p1, ctx.viewProj, ctx.viewportPos, ctx.viewportSize, sp1))
                {
                    // Kameraya bakan ön yarı-küreyi parlak, arka yarı-küreyi yarı saydam çiz
                    const bool frontFacing = (glm::dot(dir0, -viewDirToCenter) >= -0.15f);
                    ImU32 segCol = hot ? gizmo_math::kColorHover : gizmo_math::kAxisColors[i];
                    if (!hot && !frontFacing)
                    {
                        segCol = (segCol & 0x00FFFFFF) | (90u << 24);
                    }
                    dl->AddLine(sp0, sp1, segCol, hot ? 3.6f : (frontFacing ? 2.5f : 1.4f));
                }
            }
        }

        dl->AddCircleFilled(ctx.originScreen, 4.0f, IM_COL32(240, 240, 245, 220), 12);
    }
} // namespace vesta

