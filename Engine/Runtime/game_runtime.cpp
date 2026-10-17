/**
 * @file game_runtime.cpp
 * @brief Standalone (Editörsüz) oyun yürütücü uygulaması.
 */

#include "game_runtime.h"
#include "../Engine/engine.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"
#include "../Project/project.h"
#include "../Project/project_module.h"
#include "../Sera/sera.h"
#include "../Systems/physics_bridge.h"
#include "../Externals/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <GLFW/glfw3.h>

namespace runtime {

    boot_config game_runtime::load_boot_config(const std::string& config_file) {
        boot_config cfg;
        if (!std::filesystem::exists(config_file)) {
            return cfg;
        }

        try {
            std::ifstream file(config_file);
            nlohmann::json j;
            file >> j;

            cfg.project_name    = j.value("project_name", cfg.project_name);
            cfg.start_scene     = j.value("start_scene", cfg.start_scene);
            cfg.asset_directory = j.value("asset_directory", cfg.asset_directory);
            cfg.screen_width    = j.value("screen_width", cfg.screen_width);
            cfg.screen_height   = j.value("screen_height", cfg.screen_height);
            cfg.fullscreen      = j.value("fullscreen", cfg.fullscreen);
            cfg.vsync           = j.value("vsync", cfg.vsync);
        }
        catch (const std::exception& e) {
            std::cerr << "[GameRuntime] boot.config okuma hatasi: " << e.what() << "\n";
        }

        return cfg;
    }

    bool game_runtime::save_boot_config(const boot_config& config, const std::string& config_file) {
        try {
            nlohmann::json j;
            j["project_name"]    = config.project_name;
            j["start_scene"]     = config.start_scene;
            j["asset_directory"] = config.asset_directory;
            j["screen_width"]    = config.screen_width;
            j["screen_height"]   = config.screen_height;
            j["fullscreen"]      = config.fullscreen;
            j["vsync"]           = config.vsync;

            std::ofstream file(config_file);
            if (!file.is_open()) return false;
            file << j.dump(4);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    int game_runtime::run_standalone(int argc, char** argv) {
        boot_config cfg = load_boot_config("boot.config");

        EngineContext ctx;
        EditorContext editorCtx;

        ctx.isEditorBuild = false; // Saf oyun modu (Editor panelleri kapalı)
        ctx.screenW = cfg.screen_width;
        ctx.screenH = cfg.screen_height;

        if (!Engine::init(ctx, editorCtx)) {
            std::cerr << "[GameRuntime] Hata: Engine::init basarisiz.\n";
            return -1;
        }

        // Pencere başlığını oyun adı yap
        if (ctx.window) {
            glfwSetWindowTitle(ctx.window, cfg.project_name.c_str());

            if (cfg.fullscreen) {
                GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
                if (primaryMonitor) {
                    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
                    if (mode) {
                        glfwSetWindowMonitor(ctx.window, primaryMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                        ctx.screenW = mode->width;
                        ctx.screenH = mode->height;
                        glViewport(0, 0, ctx.screenW, ctx.screenH);
                    }
                }
            }

            glfwSwapInterval(cfg.vsync ? 1 : 0);
        }

        // Projeye ait dinamik GameModule.dll varsa yükle
        if (std::filesystem::exists("GameModule.dll")) {
            project::project_module::instance().load_module("GameModule.dll");
        } else if (std::filesystem::exists("Binaries/GameModule.dll")) {
            project::project_module::instance().load_module("Binaries/GameModule.dll");
        }

        // Proje varlıklarını (Assets) ARIA kayıt defterine tara ve kaydet
        const std::filesystem::path assetDir = cfg.asset_directory.empty() ? "Assets" : cfg.asset_directory;
        if (ctx.aria_system && std::filesystem::exists(assetDir)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDir)) {
                if (entry.is_regular_file() && entry.path().extension() != ".meta") {
                    const std::string filePath = entry.path().string();
                    const std::string metaPath = filePath + ".meta";
                    if (std::filesystem::exists(metaPath)) {
                        try {
                            std::ifstream mf(metaPath);
                            nlohmann::json mj;
                            mf >> mj;
                            const std::string guidStr = mj.value("guid", "");
                            if (auto parsed = uuids::uuid::from_string(guidStr)) {
                                ctx.aria_system->register_asset_path(*parsed, filePath);
                            }
                        } catch (...) {}
                    }
                }
            }
        }

        // Başlangıç sahnesini yükle ve Play moduna gir
        if (ctx.sera && std::filesystem::exists(cfg.start_scene)) {
            ctx.sera->load_scene(cfg.start_scene, ctx.session.game_scene());
        }

        ctx.session.play(ctx, false);
        physics_bridge::create_physics_world(ctx, ctx.session.game_scene());
        editorCtx.mode = EngineMode::PLAY;
        ctx.prevMode = EngineMode::PLAY;

        // Oyun döngüsü
        while (!glfwWindowShouldClose(ctx.window)) {
            Engine::update(ctx, editorCtx);
            Engine::render(ctx, editorCtx);
        }

        physics_bridge::destroy_physics_world(ctx, ctx.session.game_scene());
        ctx.session.stop();
        Engine::shutdown(ctx, editorCtx);
        project::project_module::instance().unload_module();
        return 0;
    }

} // namespace runtime

