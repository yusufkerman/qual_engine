#pragma once

/**
 * @file gizmo_scale_tool.h
 * @brief Sahne içi 3B boyut ölçekleme (Scale) gizmo aracı.
 */

#include "IGizmoTool.h"
#include "gizmo_math_utils.h"

namespace vesta
{
    /**
     * @class gizmo_scale_tool
     * @brief X, Y, Z eksen uç küpleri ve merkez tekdüze (uniform) küpü üzerinden ölçekleme manipülatörü.
     */
    class gizmo_scale_tool : public IGizmoTool
    {
    public:
        enum class Handle : int
        {
            None = 0,
            AxisX = 1,
            AxisY = 2,
            AxisZ = 3,
            Center = 4
        };

        gizmo_scale_tool() = default;
        ~gizmo_scale_tool() override = default;

        [[nodiscard]] bool is_interacting() const noexcept override { return m_Active != Handle::None; }
        void reset_interaction() override;
        void process(const gizmo_tool_context& ctx) override;

    private:
        Handle m_Hovered = Handle::None;
        Handle m_Active = Handle::None;

        gizmo_drag_state m_DragState;
        glm::vec3 m_DragAxis0{ 1.0f, 0.0f, 0.0f };
        float m_DragStartAxisParam = 0.0f;
    };
} // namespace vesta

