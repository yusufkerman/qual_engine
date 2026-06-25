#include "engine.h"

#include <iostream>

#include "../Renderer/passes/game_pass.h"
#include "../Renderer/Passes/editor_pass.h"

#include "../Editor/editor_layer.h"
#include "../Runtime/runtime_layer.h"
#include "../Systems/physics_bridge.h"
#include "../Importers/cubemap_loader.h"
#include "../Externals/nfd/nfd.h"
#include "../Renderer/Passes/ui_pass.h"
#include "../Renderer/Passes/grid_pass.h"

namespace
{
    static void framebuffer_size_callback(GLFWwindow* window, int w, int h)
    {
        auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
        ctx->screenW = w;
        ctx->screenH = h;
        glViewport(0, 0, w, h);
    }

    static void scroll_callback(GLFWwindow* window, double, double yoffset)
    {
        auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
        ctx->editor.editorCamera.scrollStep += (float)(5.0 * yoffset);
    }

    static void init_input(EngineContext& ctx)
    {
        ctx.inputMap.bind(GLFW_KEY_W, InputAction::MoveForward);
        ctx.inputMap.bind(GLFW_KEY_S, InputAction::MoveBackward);
        ctx.inputMap.bind(GLFW_KEY_A, InputAction::MoveLeft);
        ctx.inputMap.bind(GLFW_KEY_D, InputAction::MoveRight);
    }

    static void handle_mode_transition(EngineContext& ctx)
    {
        if (ctx.editor.mode == ctx.prevMode)
            return;

        ctx.editor.editorCamera.resetMouseState();
        ctx.runtimeCameraState = fly_camera_state{};

        glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        if (ctx.editor.mode == EngineMode::PLAY)
        {
            ctx.session.play();
            physics_bridge::create_physics_world(ctx, ctx.session.game_scene());
        }
        else
        {
            physics_bridge::destroy_physics_world(ctx, ctx.session.runtime_scene());
            ctx.session.stop();
        }

        ctx.prevMode = ctx.editor.mode;
    }
}

bool Engine::init(EngineContext& ctx)
{
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    ctx.window = glfwCreateWindow(ctx.screenW, ctx.screenH, "PS1 Engine", nullptr, nullptr);
    if (!ctx.window)
        return false;

    glfwMakeContextCurrent(ctx.window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return false;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetFramebufferSizeCallback(ctx.window, framebuffer_size_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);

    ctx.uiManager.init(ctx.window);
    ctx.grid.init();

    if (!ctx.physicsSystem.init())
    {
        std::cout << "[Physics] init failed\n";
        return false;
    }

    unsigned int mainShader = ctx.shaderManager.load(
        "Shaders/default_vertex.glsl",
        "Shaders/default_fragment.glsl"
    );
    std::cout << "mainShader ID: " << mainShader << std::endl;


    ctx.gridShaderID = ctx.shaderManager.load(
        "Shaders/Debug/grid.vert",
        "Shaders/Debug/grid.frag"
    );

    ctx.skyboxShaderID = ctx.shaderManager.load(
        "Shaders/Skybox/skybox.vert",
        "Shaders/Skybox/skybox.frag"
    );

    auto& s = ctx.shaderManager.get(mainShader);
    s.use();
    s.setInt("diffuseTex", 0);
    s.setInt("normalTex", 1);

    ctx.editorTarget.create(1, 1);
    ctx.gameTarget.create(1, 1);

    init_input(ctx);

    ctx.prevMode = ctx.editor.mode;

    ctx.skyboxTexture = cubemap_loader::load_atlas({
        "Assets/sky/skybox1.png"
        });

    if (NFD_Init() != NFD_OKAY)
        std::cout << "[NFD] init failed\n";

    return true;
}

void Engine::update(EngineContext& ctx)
{
    ctx.inputSystem.update(ctx.window, ctx.inputState, ctx.inputMap);

    handle_mode_transition(ctx);

    // runtime her zaman çalışır
    runtime_layer::update(ctx);

    ctx.session.game_scene().update_light_batch();
    ctx.cameraSystem.update(ctx.session.game_scene());

    // editor sadece editor build’de
    if (ctx.isEditorBuild)
    {
        std::cout << "Editor Running...\n";
        editor_layer::update(ctx);
        ctx.session.editor_scene().update_light_batch();
    }
}

void Engine::render(EngineContext& ctx)
{
    glfwPollEvents();

    game_pass::render(ctx);

    if (ctx.isEditorBuild)
    {
        editor_pass::render(ctx);
        grid_pass::render(ctx);
        editor_layer::render(ctx); // ← bunu ekle, ui_pass'ı kaldır
    }

    glfwSwapBuffers(ctx.window);
}

void Engine::shutdown(EngineContext& ctx)
{
    ctx.uiManager.shutdown();

    ctx.editorTarget.destroy();
    ctx.gameTarget.destroy();
    ctx.grid.shutdown();

    ctx.physicsSystem.shutdown();

    glfwDestroyWindow(ctx.window);
    ctx.window = nullptr;

    NFD_Quit();
    glfwTerminate();
}