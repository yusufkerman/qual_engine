#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

#include <iostream>

#include "shader.h"
#include "Importers/ModelLoader.h"
#include "Renderer/MeshRenderer.h"
#include "Systems/render_system.h"
#include "ECS/registry.h"

#include "Input/input_system.h"
#include "Input/input_state.h"
#include "Input/input_map.h"

#include "Editor/editor.h"
#include "Scene/scene_session.h"
#include "Editor/ui_manager.h"
#include <imgui_impl_glfw.h>

#include "Renderer/Debug/grid_renderer.h"
#include "Renderer/render_target.h"

static int SCREEN_W = 800;
static int SCREEN_H = 600;

static glm::vec3 CalcForward(const glm::vec3& rot)
{
    glm::vec3 forward;
    forward.x = cos(rot.x) * sin(rot.y);
    forward.y = sin(rot.x);
    forward.z = -cos(rot.x) * cos(rot.y);
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

void scroll_callback(GLFWwindow* window, double, double yoffset)
{
    auto* cam = static_cast<EditorCamera*>(glfwGetWindowUserPointer(window));
    if (!cam) return;

    cam->scrollStep += static_cast<float>(5 * yoffset);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Editor editor;
    scene_session session;

    GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "PS1 Engine", NULL, NULL);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h)
        {
            SCREEN_W = w;
            SCREEN_H = h;
            glViewport(0, 0, w, h);
        });

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // =========================
    // INPUT
    // =========================
    InputSystem inputSystem;
    InputState inputState;
    InputMap inputMap;

    inputMap.bind(GLFW_KEY_W, InputAction::MoveForward);
    inputMap.bind(GLFW_KEY_S, InputAction::MoveBackward);
    inputMap.bind(GLFW_KEY_A, InputAction::MoveLeft);
    inputMap.bind(GLFW_KEY_D, InputAction::MoveRight);

    float sensitivity = 0.0012f;
    float moveSpeed = 0.6f;

    // =========================
    // RENDER TARGETS
    // =========================
    RenderTarget editorTarget;
    RenderTarget gameTarget;
    editorTarget.create(1, 1);
    gameTarget.create(1, 1);

    // =========================
    // ENGINE
    // =========================
    Shader shader("Shaders/vertex.glsl", "Shaders/fragment.glsl");

    MeshRenderer renderer;
    render_system renderSystem;

    scene& editorScene = session.editor_scene();
    ui_manager uiManager;

    GridRenderer grid;
    Shader gridShader("Shaders/Debug/grid.vert", "Shaders/Debug/grid.frag");

    editor.setScene(&editorScene);

    uiManager.init(window);
    grid.init();

    glfwSetWindowUserPointer(window, &editor.editorCamera);
    glfwSetScrollCallback(window, scroll_callback);

    Model model = ModelLoader::Load("Assets/model1.FBX");
    unsigned int meshID = renderer.UploadModel(model);

    // MODEL
    entity chairEntity = editorScene.create_entity();
    editorScene.get_registry().add_transform(chairEntity, {
        glm::vec3(0.0f),
        glm::vec3(glm::radians(-90.0f), glm::radians(0.0f), glm::radians(0.0f)),
        glm::vec3(1.0f)
        });
    editorScene.get_registry().add_mesh(chairEntity, { meshID });

    // DEFAULT GAME CAMERA
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

    // LIGHT
    entity lightEntity = editorScene.create_entity();
    editorScene.get_registry().add_transform(lightEntity, {
        glm::vec3(25.0f, 110.0f, 5.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f)
        });
    editorScene.get_registry().add_light(lightEntity, {});

    EngineMode prevMode = editor.mode;

    while (!glfwWindowShouldClose(window))
    {
        // =========================
        // INPUT
        // =========================
        inputSystem.update(window, inputState, inputMap);

        // =========================
        // PLAY / EDIT MODE TRANSITION
        // =========================
        if (editor.mode != prevMode)
        {
            editor.editorCamera.resetMouseState();
            ResetRuntimeCameraMouseState(window);

            if (editor.mode == EngineMode::PLAY)
                session.play();
            else
                session.stop();

            prevMode = editor.mode;
        }

        

        // =========================
        // VIEWPORT RESIZE
        // =========================
        if (editor.sceneViewportSize.x > 1 && editor.sceneViewportSize.y > 1)
        {
            int newW = (int)editor.sceneViewportSize.x;
            int newH = (int)editor.sceneViewportSize.y;
            editorTarget.resize(newW, newH);
        }

        if (editor.gameViewportSize.x > 1 && editor.gameViewportSize.y > 1)
        {
            int newW = (int)editor.gameViewportSize.x;
            int newH = (int)editor.gameViewportSize.y;
            gameTarget.resize(newW, newH);
        }

        // =========================
        // CAMERA UPDATE
        // =========================
        // (FIXED)
        editor.editorCamera.update(
            window,
            inputState,
            sensitivity,
            moveSpeed
        );

        if (editor.mode == EngineMode::PLAY)
        {
            session.sync_editor_from_runtime();

            scene& gameScene = session.game_scene();
            entity activeCam = gameScene.get_active_camera();
            auto* camT = (activeCam != 0)
                ? gameScene.get_registry().get_transform(activeCam)
                : nullptr;

            UpdateRuntimeCamera(
                window,
                inputState,
                sensitivity,
                moveSpeed,
                camT,
                editor.gameViewportHovered
            );
        }

        // =========================
        // EDITOR SCENE PASS
        // =========================
        glBindFramebuffer(GL_FRAMEBUFFER, editorTarget.fbo);
        glViewport(0, 0, editorTarget.width, editorTarget.height);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 editorView = editor.editorCamera.getViewMatrix();
        glm::mat4 editorProj = editor.editorCamera.getProjectionMatrix(
            (float)editorTarget.width / (float)editorTarget.height
        );

        renderSystem.render(
            editorScene.get_registry(),
            renderer,
            shader,
            editorView,
            editorProj
        );

        // =========================
        // GRID PASS (always visible in viewport)
        // =========================
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            gridShader.use();

            const glm::vec3& camPos = editor.editorCamera.position;
            glm::vec3 gridForward = CalcForward(editor.editorCamera.rotation);

            glm::mat4 gridView = glm::lookAt(
                camPos,
                camPos + gridForward,
                glm::vec3(0, 1, 0)
            );

            glm::mat4 gridProj = glm::perspective(
                glm::radians(editor.editorCamera.fov),
                (float)editorTarget.width / (float)editorTarget.height,
                editor.editorCamera.nearPlane,
                editor.editorCamera.farPlane
            );

            glm::mat4 invViewProj = glm::inverse(gridProj * gridView);

            gridShader.setMat4("invViewProj", invViewProj);
            gridShader.setVec3("cameraPos", camPos);

            grid.draw();

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }

        // =========================
        // GAME VIEW PASS
        // =========================
        glBindFramebuffer(GL_FRAMEBUFFER, gameTarget.fbo);
        glViewport(0, 0, gameTarget.width, gameTarget.height);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        scene& gameScene = session.game_scene();
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
                    (float)gameTarget.width / (float)gameTarget.height,
                    c->nearPlane,
                    c->farPlane
                );

                renderSystem.render(
                    gameScene.get_registry(),
                    renderer,
                    shader,
                    gameView,
                    gameProj
                );
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // =========================
        // SCREEN PASS / UI
        // =========================
        glViewport(0, 0, SCREEN_W, SCREEN_H);

        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        editor.setSceneTexture(editorTarget.tex);
        editor.setGameTexture(gameTarget.tex);

        uiManager.beginFrame();

        scene& uiScene = session.is_playing() ? session.runtime_scene() : editorScene;
        editor.renderUI(uiScene);

        uiManager.endFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    editorTarget.destroy();
    gameTarget.destroy();

    glfwTerminate();
    return 0;
}