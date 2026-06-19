#include "engine.h"

#include <algorithm>
#include <iostream>
#include <cmath>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "..\Importers/ModelLoader.h"

namespace
{
    static glm::vec3 CalcForward(const glm::vec3& rot)
    {
        glm::vec3 forward;
        forward.x = std::cos(rot.x) * std::sin(rot.y);
        forward.y = std::sin(rot.x);
        forward.z = -std::cos(rot.x) * std::cos(rot.y);
        return glm::normalize(forward);
    }

    static void ResetRuntimeCameraMouseState(GLFWwindow* window)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    static void UpdateRuntimeCamera(
        GLFWwindow* window,
        InputState& input,
        float sensitivity,
        float moveSpeed,
        transform_component* cam,
        bool viewportHovered
    )
    {
        static bool runtimeMouseCaptured = false;
        static bool runtimeFirstMouse = true;
        static bool runtimePrevRightHeld = false;
        static double runtimeLastX = 0.0;
        static double runtimeLastY = 0.0;

        if (!cam)
            return;

        bool rightHeld =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        bool shiftHeld =
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        float speed = moveSpeed * (shiftHeld ? 3.0f : 1.0f);

        if (rightHeld && !runtimePrevRightHeld && viewportHovered)
        {
            runtimeMouseCaptured = true;
            runtimeFirstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        if (!rightHeld && runtimePrevRightHeld)
        {
            runtimeMouseCaptured = false;
            runtimeFirstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        runtimePrevRightHeld = rightHeld;

        if (!runtimeMouseCaptured)
            return;

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (runtimeFirstMouse)
        {
            runtimeLastX = xpos;
            runtimeLastY = ypos;
            runtimeFirstMouse = false;
        }

        float xoffset = static_cast<float>(xpos - runtimeLastX);
        float yoffset = static_cast<float>(runtimeLastY - ypos);

        runtimeLastX = xpos;
        runtimeLastY = ypos;

        cam->rotation.y += xoffset * sensitivity;
        cam->rotation.x += yoffset * sensitivity;

        cam->rotation.x = std::clamp(cam->rotation.x, -1.5f, 1.5f);

        glm::vec3 forward = CalcForward(cam->rotation);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        if (input.actions[(int)InputAction::MoveForward])
            cam->position += forward * speed;

        if (input.actions[(int)InputAction::MoveBackward])
            cam->position -= forward * speed;

        if (input.actions[(int)InputAction::MoveRight])
            cam->position += right * speed;

        if (input.actions[(int)InputAction::MoveLeft])
            cam->position -= right * speed;
    }

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

    static void init_scene_content(EngineContext& ctx)
    {
        scene& editorScene = ctx.session.editor_scene();

        ctx.editor.setScene(&editorScene);

        Model model = ModelLoader::Load("Assets/model1.FBX");
        unsigned int meshID = ctx.renderer.UploadModel(model);

        entity chairEntity = editorScene.create_entity();
        editorScene.get_registry().add_transform(chairEntity, {
            glm::vec3(0.0f),
            glm::vec3(glm::radians(-90.0f), glm::radians(0.0f), glm::radians(0.0f)),
            glm::vec3(1.0f)
            });
        editorScene.get_registry().add_mesh(chairEntity, { meshID });

        entity gameCameraEntity = editorScene.create_entity();
        editorScene.get_registry().add_transform(gameCameraEntity, {
            glm::vec3(0.0f, -5.0f, 70.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f)
            });
        editorScene.get_registry().add_camera(gameCameraEntity, {
            45.0f,
            0.1f,
            1000.0f
            });
        editorScene.set_active_camera(gameCameraEntity);

        entity lightEntity = editorScene.create_entity();
        editorScene.get_registry().add_transform(lightEntity, {
            glm::vec3(25.0f, 110.0f, 5.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f)
            });
        editorScene.get_registry().add_light(lightEntity, {});
    }

    static void handle_mode_transition(EngineContext& ctx)
    {
        if (ctx.editor.mode == ctx.prevMode)
            return;

        ctx.editor.editorCamera.resetMouseState();
        ResetRuntimeCameraMouseState(ctx.window);

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

        //ctx.session.sync_editor_from_runtime();

        scene& gameScene = ctx.session.game_scene();
        entity activeCam = gameScene.get_active_camera();

        auto* camT = (activeCam != 0)
            ? gameScene.get_registry().get_transform(activeCam)
            : nullptr;

        UpdateRuntimeCamera(
            ctx.window,
            ctx.inputState,
            ctx.sensitivity,
            ctx.moveSpeed,
            camT,
            ctx.editor.gameViewportHovered
        );
    }

    static void render_editor_pass(EngineContext& ctx)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.editorTarget.fbo);
        glViewport(0, 0, ctx.editorTarget.width, ctx.editorTarget.height);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ctx.shader->use();

        glm::mat4 editorView = ctx.editor.editorCamera.getViewMatrix();
        glm::mat4 editorProj = ctx.editor.editorCamera.getProjectionMatrix(
            (float)ctx.editorTarget.width / (float)ctx.editorTarget.height
        );

        ctx.renderSystem.render(
            ctx.session.editor_scene().get_registry(),
            ctx.renderer,
            *ctx.shader,
            editorView,
            editorProj
        );
    }

    static void render_grid_pass(EngineContext& ctx)
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        ctx.gridShader->use();

        const glm::vec3& camPos = ctx.editor.editorCamera.position;
        glm::vec3 gridForward = CalcForward(ctx.editor.editorCamera.rotation);

        glm::mat4 gridView = glm::lookAt(
            camPos,
            camPos + gridForward,
            glm::vec3(0, 1, 0)
        );

        glm::mat4 gridProj = glm::perspective(
            glm::radians(ctx.editor.editorCamera.fov),
            (float)ctx.editorTarget.width / (float)ctx.editorTarget.height,
            ctx.editor.editorCamera.nearPlane,
            ctx.editor.editorCamera.farPlane
        );

        glm::mat4 invViewProj = glm::inverse(gridProj * gridView);

        ctx.gridShader->setMat4("invViewProj", invViewProj);
        ctx.gridShader->setVec3("cameraPos", camPos);

        ctx.grid.draw();

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }

    static void render_game_pass(EngineContext& ctx)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.gameTarget.fbo);
        glViewport(0, 0, ctx.gameTarget.width, ctx.gameTarget.height);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ctx.shader->use();

        scene& gameScene = ctx.session.game_scene();
        entity gameCam = gameScene.get_active_camera();

        if (gameCam != 0 && gameScene.get_registry().has_camera(gameCam))
        {
            auto* t = gameScene.get_registry().get_transform(gameCam);
            auto* c = gameScene.get_registry().get_camera(gameCam);

            if (t && c)
            {
                glm::vec3 forward = CalcForward(t->rotation);

                glm::mat4 gameView = glm::lookAt(
                    t->position,
                    t->position + forward,
                    glm::vec3(0, 1, 0)
                );

                glm::mat4 gameProj = glm::perspective(
                    glm::radians(c->fov),
                    (float)ctx.gameTarget.width / (float)ctx.gameTarget.height,
                    c->nearPlane,
                    c->farPlane
                );

                ctx.renderSystem.render(
                    gameScene.get_registry(),
                    ctx.renderer,
                    *ctx.shader,
                    gameView,
                    gameProj
                );
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static void render_ui(EngineContext& ctx)
    {
        glViewport(0, 0, ctx.screenW, ctx.screenH);

        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        ctx.editor.setSceneTexture(ctx.editorTarget.tex);
        ctx.editor.setGameTexture(ctx.gameTarget.tex);

        ctx.uiManager.beginFrame();

        scene& uiScene = ctx.session.is_playing()
            ? ctx.session.runtime_scene()
            : ctx.session.editor_scene();

        ctx.editor.renderUI(uiScene);

        ctx.uiManager.endFrame();
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
    init_scene_content(ctx);

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
}

void Engine::render(EngineContext& ctx)
{
    render_editor_pass(ctx);
    render_grid_pass(ctx);
    render_game_pass(ctx);
    render_ui(ctx);

    glfwSwapBuffers(ctx.window);
    glfwPollEvents();
}

void Engine::shutdown(EngineContext& ctx)
{
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