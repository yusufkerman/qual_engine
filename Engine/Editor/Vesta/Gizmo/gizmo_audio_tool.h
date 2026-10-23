#pragma once

/**
 * @file gizmo_audio_tool.h
 * @brief Sahne içi etkileşimli ses kaynağı min/max etki alanı (Range) tutamaçları aracı.
 */

#include <glm/glm.hpp>
#include <imgui.h>

struct EngineContext;

namespace vesta
{
    /**
     * @class gizmo_audio_tool
     * @brief 3D uzamsal ses kaynaklarının min ve max mesafe yarıçaplarını 3B küre tutamaçları ile düzenler.
     */
    class gizmo_audio_tool
    {
    public:
        gizmo_audio_tool() = default;
        ~gizmo_audio_tool() = default;

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
        float m_AudioDragStartMinDist = 1.0f;
        float m_AudioDragStartMaxDist = 50.0f;
    };
} // namespace vesta

