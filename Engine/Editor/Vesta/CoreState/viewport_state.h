#pragma once

#include <glm/glm.hpp>

namespace vesta
{
    struct viewport_state
    {
        // ImGui panel boyutu
        glm::vec2 size{ 1920.0f, 1080.0f };

        // Gerçek framebuffer boyutu
        glm::ivec2 render_size{ 1920.0f, 1080.0f };

        glm::vec2 position{ 0.0f, 0.0f };

        unsigned int texture_id = 0;

        bool focused = false;
        bool hovered = false;
    };
}