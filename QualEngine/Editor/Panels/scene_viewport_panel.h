#pragma once

#include "../../Editor/editor_camera.h"

class scene_viewport_panel
{
public:
    void render(
        unsigned int sceneTexture,
        glm::vec2& sceneViewportSize,
        bool& sceneViewportHovered,
        EditorCamera& editorCamera);
};