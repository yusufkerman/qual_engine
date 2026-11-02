#pragma once

/**
 * @file gizmo_collider_tool.h
 * @brief Sahne içi etkileşimli Box ve Sphere çarpıştırıcı (Collider) boyutlandırma tutamaçları aracı.
 */

#include <glm/glm.hpp>
#include <imgui.h>

struct EngineContext;

namespace vesta
{
    /**
     * @class gizmo_collider_tool
     * @brief Unity tarzı Box Collider ve Sphere Collider sınırlarını ve tutamaçlarını sahne görünümünde çizip düzenler.
     */
    class gizmo_collider_tool
    {
    public:
        gizmo_collider_tool() = default;
        ~gizmo_collider_tool() = default;

        [[nodiscard]] bool is_interacting() const noexcept { return m_ActiveHandle >= 0; }
        void reset_interaction();

        void render(
            EngineContext& engineCtx,
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::vec3& camPos,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

    private:
        int m_HoveredHandle = -1;
        int m_ActiveHandle = -1;
        glm::vec3 m_DragStartExtents{ 1.0f };
    };
} // namespace vesta

