/**
 * @file engine.cpp
 * @brief Oyun motorunun temel yaşam döngüsü (başlatma, güncelleme, çizim, kapatma) işlevlerini içerir.
 */

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
#include "../Renderer/Passes/gizmo_pass.h"
#include "../Engine/asset_pipeline.h"
#include "../Sera/sera.h"
#include "../Editor/editor_context.h"
#include "../Editor/editor_camera.h"
#include "../Gameplay/gameplay_context.h"

namespace
{
    /**
     * @brief GLFW geri çağrımlarında (callbacks) kullanılmak üzere bağlam (context) verilerini tutar.
     */
    struct WindowUserData
    {
        EngineContext* engineCtx = nullptr;
        EditorContext* editorCtx = nullptr;
    };

    /**
     * @brief Pencere boyutu değiştiğinde tetiklenen geri çağrım (callback) fonksiyonu.
     */
    static void framebuffer_size_callback(GLFWwindow* window, int w, int h)
    {
        auto* data = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
        if (data && data->engineCtx)
        {
            data->engineCtx->screenW = w;
            data->engineCtx->screenH = h;
            glViewport(0, 0, w, h);
        }
    }

    /**
     * @brief Fare tekerleği kaydırıldığında tetiklenen geri çağrım (callback) fonksiyonu.
     */
    static void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset)
    {
        auto* data = static_cast<WindowUserData*>(glfwGetWindowUserPointer(window));
        if (data && data->editorCtx && data->editorCtx->camera)
        {
            data->editorCtx->camera->scrollStep += static_cast<float>(5.0 * yoffset);
        }
    }

    /**
     * @brief Temel giriş (input) tuş atamalarını (binding) yapar.
     */
    static void init_input(EngineContext& ctx)
    {
        ctx.inputMap.bind(GLFW_KEY_W, InputAction::MoveForward);
        ctx.inputMap.bind(GLFW_KEY_S, InputAction::MoveBackward);
        ctx.inputMap.bind(GLFW_KEY_A, InputAction::MoveLeft);
        ctx.inputMap.bind(GLFW_KEY_D, InputAction::MoveRight);
    }

    /**
     * @brief Oyun ve Editör modları arasındaki geçişleri ve sistem durumlarını yönetir.
     */
    static void handle_mode_transition(EngineContext& ctx, EditorContext& editorCtx)
    {
        // Mod değişikliği yoksa işlem yapma
        if (editorCtx.mode == ctx.prevMode)
            return;

        // Editör kamerası fare durumunu sıfırla
        if (editorCtx.camera)
        {
            editorCtx.camera->resetMouseState();
        }

        ctx.runtimeCameraState = fly_camera_state{};
        glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        if (editorCtx.mode == EngineMode::PLAY)
        {
            editorCtx.isPaused = false;
            ctx.gizmoSystem.ClearGameplay();
            ctx.session.play(ctx, ctx.isEditorBuild);
            physics_bridge::create_physics_world(ctx, ctx.session.game_scene());
        }
        else
        {
            editorCtx.isPaused = false;
            ctx.gizmoSystem.ClearGameplay();
            // ÖNEMLİ: Fizik dünyasını, stop() canlı sahneyi (m_liveScene) snapshot ile ezmeden önce temizle
            physics_bridge::destroy_physics_world(ctx, ctx.session.game_scene());
            ctx.session.stop();

            // Çalışma zamanında dinamik üretilmiş ve snapshot geri yüklendiğinde silinmiş varlıkları seçimden çıkar
            if (ctx.isEditorBuild)
            {
                const auto& validEntities = ctx.session.editor_scene().get_registry().get_all_entities();
                const auto selectedCopy = ctx.vestaSys.selection().get_selected_entities();
                for (const entity e : selectedCopy)
                {
                    if (validEntities.find(e) == validEntities.end())
                    {
                        ctx.vestaSys.selection().deselect_entity(e);
                    }
                }
            }
        }

        ctx.prevMode = editorCtx.mode;
    }
}

bool Engine::init(EngineContext& ctx, EditorContext& editorCtx)
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

    // OpenGL fonksiyonlarını (GLAD) yükle
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return false;

    // Aria Varlık (Asset) Sistemini başlat (Not: GLAD'den hemen sonra olmalıdır)
    if (!ctx.aria_system) {
        ctx.aria_system = new aria::system();
    }
    ctx.aria_system->init(); // Varsayılan shader ve dokuları belleğe yükler

    // Sera Sistemini başlat
    ctx.sera = new sera::sera();
    ctx.sera->init(ctx);

    // Gölge tamponunu (Shadow FBO) başlat
    if (!ctx.shadowFBO.init(2048, 2048))
    {
        std::cout << "[Engine] Hata: Gölge sistemi başlatılamadı!" << std::endl;
    }

    ctx.renderSystem.init();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // GLFW kullanıcı veri işaretçisini (User Pointer) ayarla
    static WindowUserData userData;
    userData.engineCtx = &ctx;
    userData.editorCtx = &editorCtx;

    glfwSetWindowUserPointer(ctx.window, &userData);
    glfwSetFramebufferSizeCallback(ctx.window, framebuffer_size_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);

    // Arayüz ve görsel sistemleri başlat
    ctx.uiManager.init(ctx.window);
    ctx.grid.init();
    ctx.gizmoSystem.init();

    // Fizik sistemini başlat
    if (!ctx.physicsSystem.init(ctx.session.game_scene()))
    {
        std::cout << "[Physics] Hata: init basarisiz.\n";
        return false;
    }

    // Gerekli Shader'ları Aria üzerinden içe aktar
    ctx.defaultShaderHandle = asset_pipeline::import_shader(ctx, "Shaders/default_shader");
    ctx.skinnedShaderHandle = asset_pipeline::import_shader(ctx, "Shaders/skinned_default");
    ctx.gizmoShaderHandle = asset_pipeline::import_shader(ctx, "Shaders/Debug/gizmo");
    ctx.shadowShaderHandle = asset_pipeline::import_shader(ctx, "Shaders/shadow");
    ctx.skinnedShadowShaderHandle = asset_pipeline::import_shader(ctx, "Shaders/shadow_skinned");
    ctx.gridShaderHandle = asset_pipeline::import_shader(ctx, "Shaders/Debug/grid");
    ctx.skyboxShaderHandle = asset_pipeline::import_shader(ctx, "Shaders/Skybox/skybox");

    // Ana shader için ilk uniform değerlerini bağla
    unsigned int mainGLid = ctx.aria_system->get_shader_id(ctx.defaultShaderHandle);
    if (mainGLid != 0)
    {
        glUseProgram(mainGLid);
        glUniform1i(glGetUniformLocation(mainGLid, "diffuseTex"), 0);
        glUniform1i(glGetUniformLocation(mainGLid, "normalTex"), 1);
        glUseProgram(0);
    }

    // İskelet animasyon shader'ı için ilk uniform değerlerini bağla
    unsigned int skinnedGLid = ctx.aria_system->get_shader_id(ctx.skinnedShaderHandle);
    if (skinnedGLid != 0)
    {
        glUseProgram(skinnedGLid);
        glUniform1i(glGetUniformLocation(skinnedGLid, "diffuseTex"), 0);
        glUniform1i(glGetUniformLocation(skinnedGLid, "normalTex"), 1);
        glUniform1i(glGetUniformLocation(skinnedGLid, "shadowMap"), 2);
        glUseProgram(0);
    }

    ctx.editorTarget.create(1, 1);
    ctx.gameTarget.create(1, 1);

    ctx.vestaSys.init(ctx.aria_system, ctx, editorCtx);

    // Editör modundaysa ve kamera henüz oluşturulmadıysa başlat
    if (ctx.isEditorBuild && !editorCtx.camera)
    {
        editorCtx.camera = new EditorCamera();
    }

    init_input(ctx);
    gameplay::context::set_engine_context(&ctx);
    ctx.prevMode = editorCtx.mode;

    // Gökyüzü (Skybox) küp haritasını yükle
    ctx.skyboxTexture = cubemap_loader::load_atlas("Assets/sky/skybox1.png");

    if (NFD_Init() != NFD_OKAY)
    {
        std::cout << "[NFD] Hata: init basarisiz.\n";
    }

    std::cout << "[Engine] Basariyla baslatildi. Default handle index="
        << ctx.defaultShaderHandle.index
        << " gen=" << ctx.defaultShaderHandle.generation << "\n";

    return true;
}

void Engine::update(EngineContext& ctx, EditorContext& editorCtx)
{
    gameplay::context::set_engine_context(&ctx);
    gameplay::context::set_active_scene(&ctx.session.game_scene());

    ctx.inputSystem.update(ctx, editorCtx);
    handle_mode_transition(ctx, editorCtx);

    runtime_layer::update(ctx, editorCtx);

    if (editorCtx.mode == EngineMode::PLAY && !editorCtx.isPaused)
    {
        // TODO: Sabit delta time (dt) ileride dinamik zaman ölçümüne çevrilmelidir.
        float dt = 0.016f;
        ctx.inputState.active = (!ctx.isEditorBuild) || (ctx.vestaSys.viewport_state().game().focused);
        ctx.gizmoSystem.ClearGameplay();
        ctx.gizmoSystem.SetGameplayWriteMode(true);
        ctx.session.game_scene().on_update_runtime(dt);
        ctx.gizmoSystem.SetGameplayWriteMode(false);
        ctx.animationSystem.update(ctx.session.game_scene().get_registry(), ctx.aria_system, dt);
    }

    if (ctx.isEditorBuild)
    {
        if (editorCtx.mode != EngineMode::PLAY || editorCtx.isPaused)
        {
            ctx.animationSystem.update(ctx.session.editor_scene().get_registry(), ctx.aria_system, 0.0f);
        }
        editor_layer::update(ctx, editorCtx);
    }

    // Tek canlı dünya (m_liveScene) mimarisi sayesinde ışık önbelleği kare başına yalnızca bir kez güncellenir
    ctx.session.game_scene().update_light_batch();
    ctx.cameraSystem.update(ctx.session.game_scene());
}

void Engine::render(EngineContext& ctx, EditorContext& editorCtx)
{
    glfwPollEvents();
    game_pass::render(ctx);

    if (ctx.isEditorBuild)
    {
        editor_pass::render(ctx, editorCtx);
        grid_pass::render(ctx, editorCtx);
        gizmo_pass::render(ctx, editorCtx);
        editor_layer::render(ctx, editorCtx);
    }

    glfwSwapBuffers(ctx.window);
}

void Engine::shutdown(EngineContext& ctx, EditorContext& editorCtx)
{
    ctx.session.game_scene().clean_up_scripts();
    ctx.uiManager.shutdown();

    ctx.editorTarget.destroy();
    ctx.gameTarget.destroy();
    ctx.grid.shutdown();
    ctx.physicsSystem.shutdown();

    if (editorCtx.camera)
    {
        delete editorCtx.camera;
        editorCtx.camera = nullptr;
    }

    // Sera sistemini temizle
    if (ctx.sera)
    {
        delete ctx.sera;
        ctx.sera = nullptr;
    }

    // Aria'nın tüm GPU ve bellek kaynaklarını serbest bırak
    if (ctx.aria_system) {
        ctx.aria_system->clear_all_resources();
        ctx.aria_system->shutdown();
        delete ctx.aria_system;
        ctx.aria_system = nullptr;
    }

    if (ctx.window)
    {
        glfwDestroyWindow(ctx.window);
        ctx.window = nullptr;
    }

    NFD_Quit();
    glfwTerminate();
}