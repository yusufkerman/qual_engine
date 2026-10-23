#pragma once

/**
 * @file gizmo_light_tool.h
 * @brief Sahne içi etkileşimli ışık menzili ve spot koni açısı tutamaçları aracı.
 */

#include <glm/glm.hpp>
#include <imgui.h>

struct EngineContext;

namespace vesta
{
    /**
     * @class gizmo_light_tool
     * @brief Point ve Spot ışıkların menzil (Range) ve koni açılarını (Inner/Outer Cone) 3B tutamaçlarla düzenler.
     */
    class gizmo_light_tool
    {
    public:
        gizmo_light_tool() = default;
        ~gizmo_light_tool() = default;

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
        float m_LightDragStartRange = 10.0f;
        float m_LightDragStartOuterCone = 25.0f;
        float m_LightDragStartInnerCone = 15.0f;
    };
} // namespace vesta

