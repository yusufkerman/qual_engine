#pragma once

//#include <GLFW/glfw3.h>
class GLFWwindow;

#include <glm/glm.hpp>
#include <vector>
#include <string.h>

#include "editor_camera.h"
#include "engine_mode.h"

#include "../ECS/entity.h"

#include "Panels/hierarchy_panel.h"
#include "Panels/top_bar_panel.h"
#include "Panels/scene_viewport_panel.h"
#include "Panels/game_viewport_panel.h"
#include "Panels/inspector_panel.h"

class EngineContext;
struct EditorContext;
class scene;

class Editor
{
public:
    EditorCamera editorCamera;

    glm::vec2 sceneViewportSize = { 800.0f, 600.0f };
    glm::vec2 gameViewportSize = { 800.0f, 600.0f };


    bool sceneViewportHovered = false;
    bool gameViewportHovered = false;

    EngineMode mode = EngineMode::EDITOR;

    void init(GLFWwindow* window);
    void update(scene& scene);
    void renderUI(scene& scene, EngineContext& ctx, EditorContext& editorCtx);

    void setSceneTexture(unsigned int tex) { sceneTexture = tex; }
    void setGameTexture(unsigned int tex) { gameTexture = tex; }

    void setSceneViewportSize(const glm::vec2& size) { sceneViewportSize = size; }
    void setGameViewportSize(const glm::vec2& size) { gameViewportSize = size; }

    void setScene(scene* s) { currentScene = s; }

    void toggleMode()
    {
        mode = (mode == EngineMode::EDITOR)
            ? EngineMode::PLAY
            : EngineMode::EDITOR;
    }

private:
    int selectedEntity = -1;

    unsigned int sceneTexture = 0;
    unsigned int gameTexture = 0;

    scene* currentScene = nullptr;

    int renamingEntity = -1;

    char renameBuffer[256] = {};

    std::vector<entity> deleteQueue;

    hierarchy_panel hierarchyPanel;
    top_bar_panel topBarPanel;
    scene_viewport_panel sceneViewportPanel;
    game_viewport_panel gameViewportPanel;
    inspector_panel inspectorPanel;
};