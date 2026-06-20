#include "engine.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>

#include "../Renderer/passes/editor_pass.h"
#include "../Renderer/passes/grid_pass.h"
#include "../Renderer/passes/game_pass.h"
#include "../Renderer/passes/ui_pass.h"

namespace
{
    static void framebuffer_size_callback(GLFWwindow* window, int w, int h)
    {
        auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
        if (!ctx)
            return;

        ctx->screenW = w;
        ctx->screenH = h;
        glViewport(0, 0, w, h);
    }

    static void scroll_callback(GLFWwindow* window, double, double yoffset)
    {
        auto* ctx = static_cast<EngineContext*>(glfwGetWindowUserPointer(window));
        if (!ctx)
            return;

        ctx->editor.editorCamera.scrollStep += static_cast<float>(5.0 * yoffset);
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
            ctx.session.play();
        else
            ctx.session.stop();

        ctx.prevMode = ctx.editor.mode;
    }

    static void handle_viewport_resizes(EngineContext& ctx)
    {
        if (ctx.editor.sceneViewportSize.x > 1 && ctx.editor.sceneViewportSize.y > 1)
        {
            int newW = static_cast<int>(ctx.editor.sceneViewportSize.x);
            int newH = static_cast<int>(ctx.editor.sceneViewportSize.y);
            ctx.editorTarget.resize(newW, newH);
        }

        if (ctx.editor.gameViewportSize.x > 1 && ctx.editor.gameViewportSize.y > 1)
        {
            int newW = static_cast<int>(ctx.editor.gameViewportSize.x);
            int newH = static_cast<int>(ctx.editor.gameViewportSize.y);
            ctx.gameTarget.resize(newW, newH);
        }
    }

    static void update_editor_camera(EngineContext& ctx)
    {
        ctx.editor.editorCamera.update(
            ctx.window,
            ctx.inputState,
            ctx.sensitivity,
            ctx.moveSpeed
        );
    }

    static void update_runtime_camera(EngineContext& ctx)
    {
        if (ctx.editor.mode != EngineMode::PLAY)
            return;

        scene& gameScene = ctx.session.game_scene();
        const entity activeCam = gameScene.get_active_camera();

        auto* camT = (activeCam != 0)
            ? gameScene.get_registry().get_component<transform_component>(activeCam)
            : nullptr;

        if (!camT)
            return;

        update_fly_camera(
            ctx.window,
            ctx.inputState,
            ctx.runtimeCameraState,
            camT->position,
            camT->rotation,
            ctx.sensitivity,
            ctx.moveSpeed,
            ctx.editor.gameViewportHovered,
            nullptr
        );
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

    glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetFramebufferSizeCallback(ctx.window, framebuffer_size_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);

    ctx.uiManager.init(ctx.window);
    ctx.grid.init();

    ctx.shader = std::make_unique<Shader>("Shaders/vertex.glsl", "Shaders/fragment.glsl");
    ctx.gridShader = std::make_unique<Shader>("Shaders/Debug/grid.vert", "Shaders/Debug/grid.frag");

    ctx.editorTarget.create(1, 1);
    ctx.gameTarget.create(1, 1);

    init_input(ctx);

    ctx.prevMode = ctx.editor.mode;

    return true;
}

void Engine::update(EngineContext& ctx)
{
    ctx.inputSystem.update(ctx.window, ctx.inputState, ctx.inputMap);

    handle_mode_transition(ctx);
    handle_viewport_resizes(ctx);

    update_editor_camera(ctx);

    if (ctx.editor.mode == EngineMode::PLAY)
    {
        update_runtime_camera(ctx);
        ctx.session.sync_editor_from_runtime();
    }

    ctx.cameraSystem.update(ctx.session.game_scene());
}

void Engine::render(EngineContext& ctx)
{
    editor_pass::render(ctx);
    grid_pass::render(ctx);
    game_pass::render(ctx);
    ui_pass::render(ctx);

    glfwSwapBuffers(ctx.window);
    glfwPollEvents();
}

void Engine::shutdown(EngineContext& ctx)
{
    ctx.uiManager.shutdown();

    ctx.shader.reset();
    ctx.gridShader.reset();

    ctx.editorTarget.destroy();
    ctx.gameTarget.destroy();
    ctx.grid.shutdown();

    if (ctx.window)
    {
        glfwDestroyWindow(ctx.window);
        ctx.window = nullptr;
    }

    glfwTerminate();
}