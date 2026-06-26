#pragma once

#include <glm/glm.hpp>
#include "../editor_context.h"

class game_viewport_panel
{
public:
    void render(
        unsigned int gameTexture,
        glm::vec2& gameViewportSize,
        bool& gameViewportHovered,
        EditorContext& ctx);
};