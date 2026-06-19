#pragma once

#include <glm/glm.hpp>

class game_viewport_panel
{
public:
    void render(
        unsigned int gameTexture,
        glm::vec2& gameViewportSize,
        bool& gameViewportHovered);
};