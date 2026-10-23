#pragma once

/**
 * @file gizmo_rotation_tool.h
 * @brief Sahne içi 3B yönelim döndürme (Rotate) gizmo aracı.
 */

#include "IGizmoTool.h"
#include "gizmo_math_utils.h"

namespace vesta
{
    /**
     * @class gizmo_rotation_tool
     * @brief X (Pitch), Y (Yaw) ve Z (Roll) çemberleri üzerinden teğetsel fare sürüklemesi ile döndürme manipülatörü.
     */
    class gizmo_rotation_tool : public IGizmoTool
    {
    public:
        enum class Handle : int
        {
            None = 0,
            AxisX = 1,
            AxisY = 2,
            AxisZ = 3
        };

        gizmo_rotation_tool() = default;
        ~gizmo_rotation_tool() override = default;

        [[nodiscard]] bool is_interacting() const noexcept override { return m_Active != Handle::None; }
        void reset_interaction() override;
        void process(const gizmo_tool_context& ctx) override;

    private:
        Handle m_Hovered = Handle::None;
        Handle m_Active = Handle::None;

        gizmo_drag_state m_DragState;
        glm::vec3 m_DragAxis0{ 1.0f, 0.0f, 0.0f };
        float m_CurrentRotateDeltaDeg = 0.0f;
    };
} // namespace vesta

