#pragma once

#include <glm/glm.hpp>

namespace vesta
{
    struct viewport_state
    {
        glm::vec2 size{ 800.0f, 600.0f };
        glm::vec2 position{ 0.0f, 0.0f };

        unsigned int texture_id = 0;

        bool focused = false;
        bool hovered = false;
    };
}