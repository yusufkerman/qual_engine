#pragma once

#include <glm/glm.hpp>

class Editor;
class EditorCamera;
class ui_manager;
class RenderTarget;

enum class EngineMode;

struct EditorContext
{
    Editor* editor = nullptr;

    EditorCamera* camera = nullptr;

    ui_manager* ui = nullptr;

    RenderTarget* sceneTarget = nullptr;
    RenderTarget* gameTarget = nullptr;

    glm::vec2 sceneViewportSize{ 800, 600 };
    glm::vec2 gameViewportSize{ 800, 600 };

    EngineMode* mode = nullptr;
};