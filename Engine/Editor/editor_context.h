#pragma once

#include <glm/glm.hpp>

class Editor;
class EditorCamera;
class ui_manager;
class RenderTarget;

enum class EngineMode;

enum class FocusedPanel {
	None,
	SceneViewport,
	GameViewport,
	Inspector,
	Hierarchy
};

struct EditorContext
{
    Editor* editor = nullptr;

    EditorCamera* camera = nullptr;

    ui_manager* ui = nullptr;

    RenderTarget* sceneTarget = nullptr;
    RenderTarget* gameTarget = nullptr;

    FocusedPanel focusedPanel = FocusedPanel::None;

    glm::vec2 sceneViewportSize{ 800, 600 };
    glm::vec2 gameViewportSize{ 800, 600 };
    glm::vec2 gameViewportPos = { 0.0f, 0.0f };


    EngineMode* mode = nullptr;

    int clipboardEntity = -1;
};