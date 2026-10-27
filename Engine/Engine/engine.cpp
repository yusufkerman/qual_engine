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
#include "../Project/project.h"
#include "../Project/project_module.h"
#include "../Resources/engine_resources.h"
#include "../Async/async_task_manager.h"
#include "../Core/Logger/logger.h"
#include "../Renderer/Debug/opengl_debug.h"
#include "../Core/CrashHandler/crash_handler.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#undef APIENTRY
#include <windows.h>
#endif
#include <imgui.h>

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
        ctx.inputState.cursorLocked = false;
        ctx.inputState.firstMouse = true;
        glfwSetInputMode(ctx.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        if (editorCtx.mode == EngineMode::PLAY)
        {
            // Play moduna geçmeden önce betik modülünün güncelliğini kontrol et
            if (project::project_manager::instance().has_active_project())
            {
                const std::string projRoot = project::project_manager::instance().get_project_root();
                if (project::project_module::is_module_outdated(projRoot))
                {
                    std::cout << "[Engine] Detected outdated script module before Play mode. Auto-recompiling...\n";
                    std::string log;
                    project::project_module::compile_project_scripts(projRoot, log);
                }
            }

            editorCtx.isPaused = false;
            ctx.gizmoSystem.ClearGameplay();
            ctx.session.play(ctx, ctx.isEditorBuild);
            physics_bridge::create_physics_world(ctx, ctx.session.game_scene());

            // Play moduna geçerken tüm ses kaynaklarını sıfırla ki play_on_awake baştan çalsın
            ctx.audioSystem.stop_all();
            for (auto& [e, src] : ctx.session.game_scene().get_registry().get_all<audio_source_component>())
            {
                if (src.runtime_sound_id != audio::invalid_sound_id)
                {
                    if (ctx.audioSystem.get_backend())
                    {
                        ctx.audioSystem.get_backend()->unload_sound(src.runtime_sound_id);
                    }
                    src.runtime_sound_id = audio::invalid_sound_id;
                }
                src.has_played_awake = false;
                src.is_playing = false;
                src.is_paused = false;
            }
        }
        else
        {
            editorCtx.isPaused = false;
            ctx.gizmoSystem.ClearGameplay();
            // ÖNEMLİ: Fizik ve ses dünyasını, stop() canlı sahneyi (m_liveScene) snapshot ile ezmeden önce temizle
            physics_bridge::destroy_physics_world(ctx, ctx.session.game_scene());
            ctx.audioSystem.on_stop_mode(ctx.session.game_scene().get_registry());
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

        ctx.time.reset_accumulator();
        ctx.prevMode = editorCtx.mode;
    }
}

bool Engine::init(EngineContext& ctx, EditorContext& editorCtx)
{
#ifdef _WIN32
    // Electron tabanlı editörlerin (VS Code vb.) alt süreçlerde Node.js olarak açılmasını önle
    SetEnvironmentVariableA("ELECTRON_RUN_AS_NODE", NULL);
#endif

    // 0. Post-mortem Cokme Kancalari ve Minidump Uretim Sistemini Baslat (Faz 5)
    qual::core::crash_handler::init();

    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifndef NDEBUG
    // Debug derlemesinde surucu seviyesi hata ayiklama kancasi (glDebugMessageCallback) icin debug context talep et
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    ctx.window = glfwCreateWindow(ctx.screenW, ctx.screenH, "QualEngine Editor", nullptr, nullptr);
    if (!ctx.window)
        return false;

    glfwMakeContextCurrent(ctx.window);

    // OpenGL fonksiyonlarını (GLAD) yükle
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return false;

    // OpenGL 4.5+ Surucu Seviyesi Hata Ayiklama Kancasini (glDebugMessageCallback) kur
    qual::renderer::setup_opengl_debug_output();

    // Aria Varlık (Asset) Sistemini başlat (Not: GLAD'den hemen sonra olmalıdır)
    if (!ctx.aria_system) {
        ctx.aria_system = new aria::system();
    }
    ctx.aria_system->init(); // Varsayılan shader ve dokuları belleğe yükler

    // Motor dahili kaynaklarını (Built-in Shaders & Primitives) Aria sistemine kaydet
    engine_resources::register_builtin_resources(ctx.aria_system);

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

    // Ses sistemini (Miniaudio) başlat
    if (!ctx.audioSystem.init())
    {
        std::cout << "[Audio] Uyari: Ses sistemi baslatilamadi.\n";
        return false;
    }

    // Gerekli Shader'ları Aria üzerinden içe aktar (Dahili motor kaynakları)
    ctx.defaultShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/default_shader");
    ctx.skinnedShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/skinned_default");
    ctx.gizmoShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/Debug/gizmo");
    ctx.shadowShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/shadow");
    ctx.skinnedShadowShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/shadow_skinned");
    ctx.gridShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/Debug/grid");
    ctx.skyboxShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/Skybox/skybox");

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

    if (ctx.isEditorBuild)
    {
        ctx.editorTarget.create(1, 1);
        ctx.gameTarget.create(1, 1);
        ctx.vestaSys.init(ctx.aria_system, ctx, editorCtx);
        if (!editorCtx.camera)
        {
            editorCtx.camera = new EditorCamera();
        }
    }

    init_input(ctx);
    gameplay::context::set_engine_context(&ctx);
    ctx.prevMode = editorCtx.mode;

    // Gökyüzü (Skybox) küp haritasını yükle (Dahili motor kaynağı veya proje yolu)
    std::string skyboxPath = ctx.renderSettings.skybox_path;
    if (skyboxPath.empty() || skyboxPath == "Assets/sky/skybox1.png") {
        skyboxPath = "@engine/Textures/sky/skybox1.png";
    }
    std::string resolvedSkybox = engine_resources::resolve_path(skyboxPath);
    if (std::filesystem::exists(resolvedSkybox)) {
        ctx.skyboxTexture = cubemap_loader::load_atlas(resolvedSkybox);
    } else if (std::filesystem::exists(skyboxPath)) {
        ctx.skyboxTexture = cubemap_loader::load_atlas(skyboxPath);
    }

    if (NFD_Init() != NFD_OKAY)
    {
        std::cout << "[NFD] Hata: init basarisiz.\n";
    }

    // Proje sistemini ve varsayılan proje ayarlarını (Render & Lighting) ilklendir
    project::project_manager::instance().init_default_project(ctx);

    std::cout << "[Engine] Basariyla baslatildi. Default handle index="
        << ctx.defaultShaderHandle.index
        << " gen=" << ctx.defaultShaderHandle.generation << "\n";

    return true;
}

void Engine::update(EngineContext& ctx, EditorContext& editorCtx)
{
    // 0. Arka plan asenkron görevlerden gelen ana iş parçacığı geri çağrımlarını çalıştır
    core::async_task_manager::instance().poll_main_thread_callbacks();

    // 1. Dinamik kare süresini (delta time) ve birikmiş simülasyon zamanını hesapla
    ctx.time.tick();

    gameplay::context::set_engine_context(&ctx);
    gameplay::context::set_active_scene(&ctx.session.game_scene());
    project::project_module::instance().sync_context();

    // Girdi aktiflik durumunu belirle (inputSystem.update'den ÖNCE hesaplanmalıdır)
    if (editorCtx.mode == EngineMode::PLAY && !editorCtx.isPaused)
    {
        // Editörde PLAY modundayken eğer ImGui bir metin kutusuna odaklı değilse input serbesttir
        const bool imguiTextActive = ctx.isEditorBuild && ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
        ctx.inputState.active = !imguiTextActive;
    }
    else
    {
        ctx.inputState.active = (!ctx.isEditorBuild);
    }

    handle_mode_transition(ctx, editorCtx);
    ctx.inputSystem.update(ctx, editorCtx);

    const float dt = ctx.time.delta_time();

    if (editorCtx.mode == EngineMode::PLAY && !editorCtx.isPaused)
    {
        // 2. Sabit zaman adımlı fizik ve simülasyon (Glenn Fiedler - Fix Your Timestep Accumulator)
        while (ctx.time.consume_fixed_step())
        {
            runtime_layer::fixed_update(ctx, editorCtx, ctx.time.fixed_delta_time());
            ctx.session.game_scene().on_fixed_update_runtime(ctx.time.fixed_delta_time());
        }

        // 3. Dinamik delta time ile değişken grafik ve betik güncellemeleri
        ctx.gizmoSystem.ClearGameplay();
        ctx.gizmoSystem.SetGameplayWriteMode(true);
        ctx.session.game_scene().on_update_runtime(dt);
        ctx.gizmoSystem.SetGameplayWriteMode(false);
        ctx.animationSystem.update(ctx.session.game_scene().get_registry(), ctx.aria_system, dt);
    }
    else
    {
        // Editörde veya duraklatmada birikmiş süreyi sıfırla (zaman sıçraması olmaması için)
        ctx.time.reset_accumulator();
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

    // Ses sistemini (3B dinleyici ve ses kaynakları) güncelle
    const bool isAudioPlaying = (editorCtx.mode == EngineMode::PLAY && !editorCtx.isPaused);
    scene& activeAudioScene = (editorCtx.mode == EngineMode::PLAY) ? ctx.session.game_scene() : ctx.session.editor_scene();

    if (editorCtx.mode != EngineMode::PLAY && editorCtx.camera && ctx.audioSystem.get_backend())
    {
        const glm::vec3 camPos = editorCtx.camera->position;
        const glm::vec3 camFwd = calc_forward(editorCtx.camera->rotation);
        const glm::vec3 camUp = glm::vec3(0.0f, 1.0f, 0.0f);
        ctx.audioSystem.get_backend()->set_listener_transform(camPos, camFwd, camUp);
    }

    ctx.audioSystem.update(dt, activeAudioScene.get_registry(), isAudioPlaying);
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
    if (ctx.isEditorBuild)
    {
        ctx.uiManager.shutdown();
        ctx.vestaSys.shutdown();
        ctx.editorTarget.destroy();
        ctx.gameTarget.destroy();

        if (editorCtx.camera)
        {
            delete editorCtx.camera;
            editorCtx.camera = nullptr;
        }
    }

    // Asenkron görev yöneticisini sonlandır
    core::async_task_manager::instance().shutdown();

    ctx.grid.shutdown();
    ctx.physicsSystem.shutdown();
    ctx.audioSystem.shutdown();

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