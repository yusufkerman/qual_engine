#pragma once

/**
 * @file gizmo_translation_tool.h
 * @brief Sahne içi 3B konum taşıma (Translate) gizmo aracı.
 */

#include "IGizmoTool.h"
#include "gizmo_math_utils.h"

namespace vesta
{
    /**
     * @class gizmo_translation_tool
     * @brief X, Y, Z eksenleri, 2B düzlem kutucukları (XY, XZ, YZ) ve serbest merkez üzerinden taşıma manipülatörü.
     */
    class gizmo_translation_tool : public IGizmoTool
    {
    public:
        enum class Handle : int
        {
            None = 0,
            AxisX = 1,
            AxisY = 2,
            AxisZ = 3,
            PlaneXY = 4,
            PlaneXZ = 5,
            PlaneYZ = 6,
            Center = 7
        };

        gizmo_translation_tool() = default;
        ~gizmo_translation_tool() override = default;

        [[nodiscard]] bool is_interacting() const noexcept override { return m_Active != Handle::None; }
        void reset_interaction() override;
        void process(const gizmo_tool_context& ctx) override;

    private:
        Handle m_Hovered = Handle::None;
        Handle m_Active = Handle::None;

        gizmo_drag_state m_DragState;
        glm::vec3 m_DragAxis0{ 1.0f, 0.0f, 0.0f };
        glm::vec3 m_DragPlaneNormal{ 0.0f, 0.0f, 1.0f };
        glm::vec3 m_DragStartHitPoint{ 0.0f };
        float m_DragStartAxisParam = 0.0f;
    };
} // namespace vesta

