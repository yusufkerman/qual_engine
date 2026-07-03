#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../Editor/editor.h"
#include "../Editor/editor_layer.h"

#include "../Renderer/render_target.h"
#include "../Editor/ui_manager.h"

#include "../Scene/scene_session.h"
#include "../Input/input_map.h"
#include "../Input/input_state.h"
#include "../Input/input_system.h"

#include "../Renderer/MeshRenderer.h"
#include "../Renderer/Debug/grid_renderer.h"

#include "../shader_manager.h"
#include "../Renderer/material_manager.h"
#include "../Renderer/texture_manager.h"
#include "Assets/asset_manager.h"

#include "../Systems/physics_system.h"
#include "../Systems/render_system.h"
#include "../Systems/camera_system.h"

#include <memory>
#include "../Importers/ModelLoader.h"
#include "../Renderer/shadow_map_fbo.h"
#include "../Renderer/Debug/gizmo_system.h"

#include "Assets/aria.h"

struct GLFWwindow;

namespace sera
{
    class sera;
}

struct EngineContext
{
    GLFWwindow* window = nullptr;

    int screenW = 1280;
    int screenH = 720;

    float sensitivity = 0.002f;
    float moveSpeed = 0.10f;

    bool isEditorBuild = true;


    // ---------------- CORE ----------------
    InputState inputState;
    InputMap inputMap;
    InputSystem inputSystem;

    scene_session session;

    sera::sera* sera = nullptr;

    // ---------------- EDITOR ----------------
    Editor editor;
    ui_manager uiManager;

    RenderTarget editorTarget;
    RenderTarget gameTarget;

    EngineMode prevMode = EngineMode::EDITOR;

    // ---------------- RENDER / SYSTEMS ----------------
    MeshRenderer renderer;
    GridRenderer grid;

    shader_manager shaderManager;
    material_manager materialManager;
    texture_manager textureManager;
    asset_manager assetManager;

    render_system renderSystem;
    camera_system cameraSystem;
    physics_system physicsSystem;
    aria::system* aria_system = nullptr;

    gizmo_system gizmoSystem;

    ModelLoader modelLoader;

    shadow_map_fbo shadowFBO;

    // runtime
    /*unsigned int gridShaderID = 0;*/
    //unsigned int skyboxShaderID = 0;
    unsigned int skyboxTexture = 0;
    //unsigned int shadowShaderID = 0;
    //unsigned int gizmoShaderID = 0;

	aria::handle<aria::shader_asset> gridShaderHandle;
    aria::handle<aria::shader_asset> skyboxShaderHandle;
    aria::handle<aria::shader_asset> defaultShaderHandle;
    aria::handle<aria::shader_asset> gizmoShaderHandle;
    aria::handle<aria::shader_asset> shadowShaderHandle;

    fly_camera_state runtimeCameraState;


};