/**
 * @file gizmo_scale_tool.cpp
 * @brief Sahne içi 3B boyut ölçekleme (Scale) gizmo aracı uygulaması.
 */

#include "gizmo_scale_tool.h"
#include "../../../Engine/engine_context.h"
#include "../../../Renderer/Debug/gizmo_settings.h"

namespace vesta
{
    void gizmo_scale_tool::reset_interaction()
    {
        m_Active = Handle::None;
        m_Hovered = Handle::None;
        m_DragState.clear();
    }

    void gizmo_scale_tool::process(const gizmo_tool_context& ctx)
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

            // Merkez Uniform Scale küpü
            if (std::hypot(ctx.mousePos.x - ctx.originScreen.x, ctx.mousePos.y - ctx.originScreen.y) <= 10.0f)
            {
                m_Hovered = Handle::Center;
            }
            else
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

            if (m_Hovered != Handle::None && ctx.isMouseClicked)
            {
                m_Active = m_Hovered;
                m_DragState.capture(ctx.activeEntities, ctx.primaryEntity, ctx.originWorld, ctx.mousePos, reg);

                if (m_Active != Handle::Center)
                {
                    const int idx = static_cast<int>(m_Active) - static_cast<int>(Handle::AxisX);
                    m_DragAxis0 = axes[idx];
                    m_DragStartAxisParam = gizmo_math::ray_axis_closest_param(mouseRay, ctx.originWorld, m_DragAxis0);
                }
            }
        }

        // 2. Aktif Sürükleme Hesaplaması
        if (m_Active != Handle::None && ctx.isMouseDown && !m_DragState.empty())
        {
            if (m_Active == Handle::Center)
            {
                const float mouseDelta = (ctx.mousePos.x - m_DragState.startMousePos.x) - (ctx.mousePos.y - m_DragState.startMousePos.y);
                float factor = 1.0f + mouseDelta * 0.0075f;
                if (ctx.snapActive) {
                    factor = gizmo_math::snap_value(factor, settings.scale_snap);
                }
                factor = std::max(0.01f, factor);

                for (size_t i = 0; i < m_DragState.dragEntities.size(); ++i)
                {
                    const entity ent = m_DragState.dragEntities[i];
                    auto* t = reg.get_component<transform_component>(ent);
                    if (!t) continue;

                    const glm::vec3 offset = m_DragState.startWorldPositions[i] - m_DragState.startWorldPos;
                    const glm::vec3 scaledOffset = offset * factor;
                    const glm::vec3 targetWorld = m_DragState.startWorldPos + scaledOffset;

                    gizmo_math::apply_entity_world_position(ent, targetWorld, reg);
                    t->scale = glm::max(glm::vec3(0.001f), m_DragState.startScales[i] * factor);
                }

                if (m_DragState.dragEntities.size() > 1)
                {
                    ImGui::SetTooltip("Scaling %zu Entities (Uniform)\nFactor: %.2fx", m_DragState.dragEntities.size(), factor);
                }
                else
                {
                    const auto* primaryTransform = reg.get_component<transform_component>(ctx.primaryEntity);
                    if (primaryTransform)
                    {
                        ImGui::SetTooltip("Scale: (%.2f, %.2f, %.2f)", primaryTransform->scale.x, primaryTransform->scale.y, primaryTransform->scale.z);
                    }
                }
            }
            else
            {
                const int idx = static_cast<int>(m_Active) - static_cast<int>(Handle::AxisX);
                const float curParam = gizmo_math::ray_axis_closest_param(mouseRay, m_DragState.startWorldPos, m_DragAxis0);
                float delta = (curParam - m_DragStartAxisParam) / std::max(0.1f, ctx.gizmoWorldScale);
                if (ctx.snapActive) {
                    delta = gizmo_math::snap_value(delta, settings.scale_snap);
                }

                for (size_t i = 0; i < m_DragState.dragEntities.size(); ++i)
                {
                    const entity ent = m_DragState.dragEntities[i];
                    auto* t = reg.get_component<transform_component>(ent);
                    if (!t) continue;

                    t->scale[idx] = std::max(0.01f, m_DragState.startScales[i][idx] + delta);
                }

                if (m_DragState.dragEntities.size() > 1)
                {
                    ImGui::SetTooltip("Scaling %zu Entities (Axis %s)\nDelta: %+.2f",
                        m_DragState.dragEntities.size(), idx == 0 ? "X" : (idx == 1 ? "Y" : "Z"), delta);
                }
                else
                {
                    const auto* primaryTransform = reg.get_component<transform_component>(ctx.primaryEntity);
                    if (primaryTransform)
                    {
                        ImGui::SetTooltip("Scale: (%.2f, %.2f, %.2f)", primaryTransform->scale.x, primaryTransform->scale.y, primaryTransform->scale.z);
                    }
                }
            }
        }

        // 3. Çizim: X, Y, Z Ölçek Kolları ve Uç Küpleri
        for (int i = 0; i < 3; ++i)
        {
            if (!tipValid[i]) continue;
            const Handle id = static_cast<Handle>(static_cast<int>(Handle::AxisX) + i);
            const bool hot = (m_Hovered == id) || (m_Active == id);
            const ImU32 col = hot ? gizmo_math::kColorHover : gizmo_math::kAxisColors[i];

            dl->AddLine(ctx.originScreen, tipScreen[i], col, hot ? 3.8f : 2.8f);

            const float boxSz = hot ? 6.5f : 5.0f;
            dl->AddRectFilled(
                ImVec2(tipScreen[i].x - boxSz, tipScreen[i].y - boxSz),
                ImVec2(tipScreen[i].x + boxSz, tipScreen[i].y + boxSz),
                col, 1.5f);
            dl->AddRect(
                ImVec2(tipScreen[i].x - boxSz, tipScreen[i].y - boxSz),
                ImVec2(tipScreen[i].x + boxSz, tipScreen[i].y + boxSz),
                IM_COL32(20, 22, 28, 220), 1.5f, 0, 1.2f);
        }

        // Merkez Uniform Scale Küpü
        const bool centerHot = (m_Hovered == Handle::Center) || (m_Active == Handle::Center);
        const float cBox = centerHot ? 7.5f : 6.0f;
        dl->AddRectFilled(
            ImVec2(ctx.originScreen.x - cBox, ctx.originScreen.y - cBox),
            ImVec2(ctx.originScreen.x + cBox, ctx.originScreen.y + cBox),
            centerHot ? gizmo_math::kColorHover : IM_COL32(245, 245, 250, 245), 1.5f);
        dl->AddRect(
            ImVec2(ctx.originScreen.x - cBox, ctx.originScreen.y - cBox),
            ImVec2(ctx.originScreen.x + cBox, ctx.originScreen.y + cBox),
            IM_COL32(20, 22, 28, 235), 1.5f, 0, 1.5f);
    }
} // namespace vesta

