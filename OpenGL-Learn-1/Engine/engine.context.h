#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "..\Editor/editor.h"
#include "..\Scene/scene_session.h"
#include "..\Input/input_system.h"
#include "..\Input/input_state.h"
#include "..\Input/input_map.h"
#include "..\Renderer/MeshRenderer.h"
#include "..\Systems/render_system.h"
#include "..\Renderer/Debug/grid_renderer.h"
#include "..\Renderer/render_target.h"
#include "..\Editor/ui_manager.h"
#include "..\shader.h"

struct EngineContext
{
    GLFWwindow* window = nullptr;

    int screenW = 800;
    int screenH = 600;

    Editor editor;
    scene_session session;

    InputSystem inputSystem;
    InputState inputState;
    InputMap inputMap;

    RenderTarget editorTarget;
    RenderTarget gameTarget;

    MeshRenderer renderer;
    render_system renderSystem;

    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> gridShader;

    GridRenderer grid;
    ui_manager uiManager;

    float sensitivity = 0.0012f;
    float moveSpeed = 0.6f;

    EngineMode prevMode = EngineMode::EDITOR;
};