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
            ctx.session.play(ctx);
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

    if (!ctx.shadowFBO.init(2048, 2048))
    {
        std::cout << "Gölge sistemi başlatılamadı!" << std::endl;
    }

    ctx.textureManager.init();
    ctx.renderSystem.init();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetFramebufferSizeCallback(ctx.window, framebuffer_size_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);

    ctx.uiManager.init(ctx.window);
    ctx.grid.init();


    if (!ctx.physicsSystem.init(ctx.session.game_scene()))
    {
        std::cout << "[Physics] init failed\n";
        return false;
    }

   

    unsigned int mainShaderID = ctx.shaderManager.load(
        "Shaders/default_vertex.glsl",
        "Shaders/default_fragment.glsl"
    );
    ctx.renderSystem.bind_light_buffer(ctx.shaderManager.get(mainShaderID));

    //Gölge shader'ı yüklenir.
    ctx.shadowShaderID = ctx.shaderManager.load(
        "Shaders/shadow_vertex.glsl",
        "Shaders/shadow_fragment.glsl"
    );


    ctx.gridShaderID = ctx.shaderManager.load(
        "Shaders/Debug/grid.vert",
        "Shaders/Debug/grid.frag"
    );

    ctx.skyboxShaderID = ctx.shaderManager.load(
        "Shaders/Skybox/skybox.vert",
        "Shaders/Skybox/skybox.frag"
    );

    auto& s = ctx.shaderManager.get(mainShaderID);
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

void Engine::update(EngineContext& ctx, EditorContext& editorCtx)
{
    ctx.inputSystem.update(ctx, editorCtx);

    handle_mode_transition(ctx);

    // runtime her zaman çalışır
    runtime_layer::update(ctx);

    if (ctx.editor.mode == EngineMode::PLAY)
    {
        float dt = 0.016f; //Geçici

        ctx.inputState.active = (!ctx.isEditorBuild) || (editorCtx.focusedPanel == FocusedPanel::GameViewport);

        ctx.session.game_scene().on_update_runtime(dt);
    }

    ctx.session.game_scene().update_light_batch();
    ctx.cameraSystem.update(ctx.session.game_scene());

    // editor sadece editor build’de
    if (ctx.isEditorBuild)
    {
        editor_layer::update(ctx, editorCtx);
        ctx.session.editor_scene().update_light_batch();
    }
}

void Engine::render(EngineContext& ctx, EditorContext& editorCtx)
{
    glfwPollEvents();

    game_pass::render(ctx);

    if (ctx.isEditorBuild)
    {
        editor_pass::render(ctx);
        grid_pass::render(ctx);
        editor_layer::render(ctx, editorCtx); // ← bunu ekle, ui_pass'ı kaldır
    }

    glfwSwapBuffers(ctx.window);
}

void Engine::shutdown(EngineContext& ctx)
{
    ctx.session.game_scene().clean_up_scripts();

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