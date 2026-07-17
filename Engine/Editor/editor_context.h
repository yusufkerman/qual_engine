#pragma once

#include <glm/glm.hpp>
#include <string>

#include "../Editor/engine_mode.h"

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
	Hierarchy,
    AssetBrowser
};

struct EditorContext
{
    EditorCamera* camera = nullptr;

    ui_manager* ui = nullptr;

    RenderTarget* sceneTarget = nullptr;
    RenderTarget* gameTarget = nullptr;

    FocusedPanel focusedPanel = FocusedPanel::None;


    unsigned int sceneTexture = 0;
    unsigned int gameTexture = 0;

    void setSceneTexture(unsigned int tex) { sceneTexture = tex; }
    void setGameTexture(unsigned int tex) { gameTexture = tex; }


    EngineMode mode = EngineMode::EDITOR;

    int clipboardEntity = -1;
    std::string selectedAssetPath;
};