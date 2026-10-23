#pragma once

/**
 * @file gizmo_rect_tool.h
 * @brief 2B Oyun İçi UI dikdörtgen boyutlandırma ve taşıma aracı (Rect Tool).
 */

#include <glm/glm.hpp>
#include <imgui.h>
#include "../../../ECS/entity.h"

struct EngineContext;

namespace vesta
{
    /**
     * @class gizmo_rect_tool
     * @brief rect_transform_component bileşenine sahip 2B UI öğelerini ekran uzayında 8 tutamaç ile manipüle eder.
     */
    class gizmo_rect_tool
    {
    public:
        enum class Handle : int
        {
            None = 0,
            Body = 1,
            TopLeft = 2,
            TopRight = 3,
            BottomLeft = 4,
            BottomRight = 5,
            Left = 6,
            Right = 7,
            Top = 8,
            Bottom = 9
        };

        gizmo_rect_tool() = default;
        ~gizmo_rect_tool() = default;

        [[nodiscard]] bool is_interacting() const noexcept { return m_Active != Handle::None; }
        void reset_interaction();

        void render(
            EngineContext& engineCtx,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

    private:
        Handle m_Hovered = Handle::None;
        Handle m_Active = Handle::None;

        entity m_DragEntity = 0;
        glm::vec3 m_DragStartPosition{ 0.0f };
        glm::vec3 m_DragStartScale{ 1.0f };
        glm::vec2 m_DragStartMousePos{ 0.0f };
    };
} // namespace vesta

