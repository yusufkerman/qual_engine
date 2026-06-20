#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../Editor/editor.h"
#include "../Engine/fly_camera.h"
#include "../Input/input_map.h"
#include "../Input/input_state.h"
#include "../Input/input_system.h"
#include "../Renderer/MeshRenderer.h"
#include "../Renderer/Debug/grid_renderer.h"
#include "../Renderer/render_target.h"
#include "../Systems/render_system.h"
#include "../Scene/scene_session.h"
#include "../Editor/ui_manager.h"
#include "../shader_manager.h"

#include <memory>
#include "../Renderer/material_manager.h"
#include "../Renderer/texture_manager.h"

struct GLFWwindow;
class shader_manager;

struct EngineContext
{
    GLFWwindow* window = nullptr;

    int screenW = 1280;
    int screenH = 720;

    float sensitivity = 0.002f;
    float moveSpeed = 0.10f;

    unsigned int gridShaderID;

    InputState inputState;
    InputMap inputMap;
    InputSystem inputSystem;

    Editor editor;
    scene_session session;

    MeshRenderer renderer;
    GridRenderer grid;
    RenderTarget editorTarget;
    RenderTarget gameTarget;
    ui_manager uiManager;

    shader_manager shaderManager;
    material_manager materialManager;
    texture_manager textureManager;

    render_system renderSystem;
    camera_system cameraSystem;

    EngineMode prevMode = EngineMode::EDITOR;
    fly_camera_state runtimeCameraState;

};