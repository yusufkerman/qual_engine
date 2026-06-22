#include "engine.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>

#include "../Renderer/passes/editor_pass.h"
#include "../Renderer/passes/grid_pass.h"
#include "../Renderer/passes/game_pass.h"
#include "../Renderer/passes/ui_pass.h"
#include "../Importers/cubemap_loader.h"
#include "../Renderer/Passes/skybox_pass.h"
#include "../Systems/physics_bridge.h"
#include "../Externals/nfd/nfd.h"
#include <iostream>

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

        if (ctx.editor.mode == EngineMode::PLAY) {
            ctx.session.play();
            physics_bridge::create_physics_world(ctx, ctx.session.game_scene());
        }
        else {
            // ⚠️ KRİTİK DÜZELTME:
            // Önce runtime_scene'i el ile vererek Jolt dünyasını ve rigidBody bileşenlerini tamamen yok ediyoruz!
            physics_bridge::destroy_physics_world(ctx, ctx.session.runtime_scene());

            // Jolt tamamen temizlendikten sonra sahneyi snapshot'tan geri yükleyebiliriz
            ctx.session.stop();
        }

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

    // Jolt Fizik Sistemini Başlatıyoruz
    if (!ctx.physicsSystem.init())
    {
        std::cout << "[Jolt Error] Fizik sistemi baslatilamadi!" << std::endl;
        return false;
    }

    unsigned int mainShader = ctx.shaderManager.load(
        "Shaders/default_vertex.glsl",
        "Shaders/default_fragment.glsl"
    );

    ctx.gridShaderID = ctx.shaderManager.load(
        "Shaders/Debug/grid.vert",
        "Shaders/Debug/grid.frag"
    );

    ctx.skyboxShaderID = ctx.shaderManager.load(
        "Shaders/Skybox/skybox.vert",
        "Shaders/Skybox/skybox.frag"
    );
	

    shader& s = ctx.shaderManager.get(mainShader);
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
    {
        std::cout << "[NFD Error] Dosya gezgini baslatilamadi!" << std::endl;
    }

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

        // ⚠️ 1. ADIM: Önce editör sahnesinde (Inspector'da) yaptığın değişiklikleri runtime (game) sahnesine aktar!
        // (Eğer senin yapında tam tersi sync_runtime_from_editor gibi bir fonksiyon varsa buraya o gelmeli)

        // 2. ADIM: Game scene'deki son transformları Jolt'a gönder
        physics_bridge::sync_ecs_to_physics(ctx, ctx.session.game_scene());

        // 3. ADIM: Jolt simülasyonu 1 kare ilerletsin
        ctx.physicsSystem.update(0.016f);

        // 4. ADIM: Jolt'ta oluşan yeni yerleri game_scene transformuna yaz
        physics_bridge::sync_physics_to_ecs(ctx, ctx.session.game_scene());

        // ⚠️ 5. ADIM: Runtime'da (Jolt ile) hareket eden topun pozisyonunu editöre bildir ki editör ekranında da topun düştüğünü görebilesin.
        ctx.session.sync_editor_from_runtime();
    }

    ctx.session.editor_scene().update_light_batch();
    ctx.session.game_scene().update_light_batch();

    ctx.cameraSystem.update(ctx.session.game_scene());
}

void Engine::render(EngineContext& ctx)
{
    glfwPollEvents();

    editor_pass::render(ctx);
    grid_pass::render(ctx);

    game_pass::render(ctx);

    ui_pass::render(ctx);

    glfwSwapBuffers(ctx.window);   
}

void Engine::shutdown(EngineContext& ctx)
{
    ctx.uiManager.shutdown();

    ctx.editorTarget.destroy();
    ctx.gameTarget.destroy();
    ctx.grid.shutdown();

    ctx.physicsSystem.shutdown();

    if (ctx.window)
    {
        glfwDestroyWindow(ctx.window);
        ctx.window = nullptr;
    }

    NFD_Quit();

    glfwTerminate();
}