/**
 * @file project_exporter.cpp
 * @brief Standalone oyun dışa aktarma (Export) servisi uygulaması.
 */

#include "project_exporter.h"
#include "project.h"
#include "project_module.h"
#include "../Runtime/game_runtime.h"
#include "../Resources/engine_resources.h"
#include "../Externals/nlohmann/json.hpp"
#if __has_include("qual_build_config.h")
#include "qual_build_config.h"
#endif
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace project {

    namespace {

        std::filesystem::path get_current_exe_dir() {
            char buffer[MAX_PATH];
            DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
            if (len > 0) {
                return std::filesystem::path(buffer).parent_path();
            }
            return std::filesystem::current_path();
        }

        std::filesystem::path find_game_runtime_exe() {
            const auto exeDir = get_current_exe_dir();
            const auto cwd = std::filesystem::current_path();

            const std::filesystem::path candidates[] = {
                exeDir / "GameRuntime.exe",
#ifdef QUAL_OUTPUT_ROOT
                std::filesystem::path(QUAL_OUTPUT_ROOT) / "Release" / "GameRuntime.exe",
                std::filesystem::path(QUAL_OUTPUT_ROOT) / "Debug" / "GameRuntime.exe",
#endif
                exeDir / ".." / "x64" / "Release" / "GameRuntime.exe",
                exeDir / ".." / "x64" / "Debug" / "GameRuntime.exe",
                cwd / "x64" / "Release" / "GameRuntime.exe",
                cwd / "x64" / "Debug" / "GameRuntime.exe"
            };

            for (const auto& c : candidates) {
                std::error_code ec;
                if (std::filesystem::exists(c, ec) && !std::filesystem::is_directory(c, ec)) {
                    return std::filesystem::canonical(c);
                }
            }

            return {};
        }

        std::filesystem::path find_assimp_dll() {
            const auto exeDir = get_current_exe_dir();
            const auto cwd = std::filesystem::current_path();

            const std::filesystem::path candidates[] = {
                exeDir / "assimp-vc145-mt.dll",
#ifdef QUAL_ASSIMP_DLL
                std::filesystem::path(QUAL_ASSIMP_DLL),
#endif
                cwd / "x64" / "Release" / "assimp-vc145-mt.dll",
                cwd / "x64" / "Debug" / "assimp-vc145-mt.dll",
                "C:\\dev\\assimp-6.0.5\\assimp_build\\bin\\Release\\assimp-vc145-mt.dll",
                "C:\\dev\\assimp-6.0.5\\assimp_build\\bin\\Debug\\assimp-vc145-mt.dll"
            };

            for (const auto& c : candidates) {
                std::error_code ec;
                if (std::filesystem::exists(c, ec)) {
                    return std::filesystem::canonical(c);
                }
            }

            return {};
        }

    } // anonymous namespace

    export_result project_exporter::export_project(const export_options& options) {
        export_result result;

        // 1. Proje geçerlilik kontrolü
        auto& pm = project_manager::instance();
        if (!pm.has_active_project()) {
            result.error_message = "Hata: Aktif acik bir proje bulunamadi.";
            return result;
        }

        if (options.output_directory.empty()) {
            result.error_message = "Hata: Cikti dizini belirtilmedi.";
            return result;
        }

        try {
            const std::filesystem::path outDir(options.output_directory);
            const std::filesystem::path projectRoot(pm.get_project_root());
            const auto& desc = pm.get_descriptor();

            result.log_messages.push_back("[ProjectExporter] Export baslatildi: " + desc.name);
            result.log_messages.push_back("[ProjectExporter] Hedef dizin: " + outDir.string());

            // 2. Hedef dizini oluştur
            std::filesystem::create_directories(outDir);

            // 3. Proje Scriptlerini Derle (Gerekliyse)
            if (options.compile_scripts) {
                result.log_messages.push_back("[ProjectExporter] Proje betikleri derleniyor (GameModule.dll)...");
                std::string compileLog;
                bool compileSuccess = project_module::compile_project_scripts(projectRoot.string(), compileLog);
                result.log_messages.push_back(compileLog);

                if (!compileSuccess) {
                    result.error_message = "Hata: Betik derlemesi basarisiz oldu. Export durduruldu.";
                    return result;
                }
            }

            // 4. GameModule.dll ve sembol dosyasını kopyala
            const std::filesystem::path moduleSrc = projectRoot / "Binaries" / "GameModule.dll";
            if (std::filesystem::exists(moduleSrc)) {
                std::error_code ec;
                std::filesystem::copy_file(moduleSrc, outDir / "GameModule.dll", std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    result.log_messages.push_back("[ProjectExporter] Uyari: GameModule.dll kopyalanamadi: " + ec.message());
                } else {
                    result.log_messages.push_back("[ProjectExporter] GameModule.dll basariyla kopyalandi.");
                }

                const std::filesystem::path pdbSrc = projectRoot / "Binaries" / "GameModule.pdb";
                if (std::filesystem::exists(pdbSrc)) {
                    std::filesystem::copy_file(pdbSrc, outDir / "GameModule.pdb", std::filesystem::copy_options::overwrite_existing, ec);
                }
            } else {
                result.log_messages.push_back("[ProjectExporter] Bilgi: Projede GameModule.dll bulunamadi (Saf sahne projesi).");
            }

            // 5. GameRuntime.exe dosyasını tespit et ve <ProjeAdi>.exe olarak kopyala
            const auto runtimeExe = find_game_runtime_exe();
            if (runtimeExe.empty()) {
                result.error_message = "Hata: GameRuntime.exe bulunamadi! Once GameRuntime projesini derleyin.";
                return result;
            }

            const std::string targetExeName = desc.name + ".exe";
            const std::filesystem::path targetExePath = outDir / targetExeName;

            std::error_code ec;
            std::filesystem::copy_file(runtimeExe, targetExePath, std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                result.error_message = "Hata: GameRuntime.exe kopyalanamadi: " + ec.message();
                return result;
            }
            result.target_executable = targetExePath.string();
            result.log_messages.push_back("[ProjectExporter] Calistirilabilir dosya olusturuldu: " + targetExeName);

            // 6. boot.config dosyasını oluştur
            runtime::boot_config bootCfg;
            bootCfg.project_name    = desc.name;
            bootCfg.start_scene     = desc.start_scene;
            bootCfg.asset_directory = desc.asset_directory;
            bootCfg.screen_width    = options.screen_width;
            bootCfg.screen_height   = options.screen_height;
            bootCfg.fullscreen      = options.fullscreen;
            bootCfg.vsync           = desc.rendering.vsync;

            const std::filesystem::path bootConfigPath = outDir / "boot.config";
            if (!runtime::game_runtime::save_boot_config(bootCfg, bootConfigPath.string())) {
                result.log_messages.push_back("[ProjectExporter] Uyari: boot.config kaydedilemedi.");
            } else {
                result.log_messages.push_back("[ProjectExporter] boot.config basariyla yazildi.");
            }

            // 7. Proje Varlıklarını (Assets) Kopyala
            const std::filesystem::path assetsSrc = projectRoot / desc.asset_directory;
            if (std::filesystem::exists(assetsSrc)) {
                const std::filesystem::path assetsDst = outDir / desc.asset_directory;
                std::filesystem::create_directories(assetsDst);
                std::filesystem::copy(assetsSrc, assetsDst, 
                    std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
                result.log_messages.push_back("[ProjectExporter] Proje varliklari kopyalandi (" + desc.asset_directory + ").");
            }

            // 8. Motor Dahili Kaynaklarını (EngineResources) Kopyala
            if (options.copy_engine_resources) {
                const auto& engineResSrc = engine_resources::get_resources_path();
                if (std::filesystem::exists(engineResSrc)) {
                    const std::filesystem::path engineResDst = outDir / "EngineResources";
                    std::filesystem::create_directories(engineResDst);
                    std::filesystem::copy(engineResSrc, engineResDst,
                        std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
                    result.log_messages.push_back("[ProjectExporter] Motor dahili kaynaklari kopyalandi (EngineResources).");
                }
            }

            // 8.1. asset_manifest.json Dosyasını Oluştur
            try {
                nlohmann::json manifestJson = nlohmann::json::object();

                // Dahili Motor Kaynakları (Shaders & Primitives)
                manifestJson["1a96340a-1192-4f27-b8fd-0439b686f63b"] = (outDir / "EngineResources" / "Shaders" / "Debug" / "gizmo").string();
                manifestJson["ab217918-3c47-43c5-b01e-e4a5feae6a82"] = (outDir / "EngineResources" / "Shaders" / "Skybox" / "skybox").string();
                manifestJson["c10d2850-634a-4e41-bd53-add2e817e129"] = (outDir / "EngineResources" / "Shaders" / "default_shader").string();
                manifestJson["ca6418ad-3833-40aa-9847-5bbc212a890a"] = (outDir / "EngineResources" / "Shaders" / "Debug" / "grid").string();
                manifestJson["cd5743f9-dbda-4c13-8a08-c41f6c9bfa71"] = (outDir / "EngineResources" / "Shaders" / "skinned_default").string();
                manifestJson["d23a0aff-3fdc-5851-a6c3-e9623da3cd11"] = "engine://builtin/cube";
                manifestJson["dab2ae4c-56df-4dcb-a781-82fabfacee3d"] = (outDir / "EngineResources" / "Shaders" / "shadow").string();
                manifestJson["e5c2194b-4b19-4824-a749-33b684cb32fa"] = (outDir / "EngineResources" / "Shaders" / "shadow_skinned").string();
                manifestJson["f18b3e21-9541-4770-bc2d-5b3a8829f001"] = "engine://builtin/sphere";

                // Proje Varlıkları (.meta dosyalarından GUID ve dosya yolları)
                const std::filesystem::path assetsDst = outDir / desc.asset_directory;
                if (std::filesystem::exists(assetsDst)) {
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsDst)) {
                        if (entry.is_regular_file() && entry.path().extension() == ".meta") {
                            const std::string metaFilePath = entry.path().string();
                            const std::string assetFilePath = metaFilePath.substr(0, metaFilePath.size() - 5);
                            if (std::filesystem::exists(assetFilePath)) {
                                try {
                                    std::ifstream mf(metaFilePath);
                                    nlohmann::json mj;
                                    mf >> mj;
                                    const std::string guidStr = mj.value("guid", "");
                                    if (!guidStr.empty()) {
                                        manifestJson[guidStr] = assetFilePath;
                                    }
                                } catch (...) {}
                            }
                        }
                    }
                }

                std::ofstream manifestFile(outDir / "asset_manifest.json");
                if (manifestFile.is_open()) {
                    manifestFile << manifestJson.dump(4);
                    manifestFile.close();
                    result.log_messages.push_back("[ProjectExporter] asset_manifest.json basariyla olusturuldu.");
                }
            } catch (...) {}

            // 9. Gerekli DLL Bağımlılıklarını Kopyala
            if (options.copy_dependencies) {
                const auto assimpDll = find_assimp_dll();
                if (!assimpDll.empty()) {
                    std::filesystem::copy_file(assimpDll, outDir / "assimp-vc145-mt.dll",
                        std::filesystem::copy_options::overwrite_existing, ec);
                    result.log_messages.push_back("[ProjectExporter] assimp-vc145-mt.dll kopyalandi.");
                }
            }

            // 10. Editör oturumunda betiklerin ve GameModule.dll'in canlı kalmasını garantiye al
            if (std::filesystem::exists(moduleSrc)) {
                project_module::instance().load_module(moduleSrc.string());
                result.log_messages.push_back("[ProjectExporter] Editor icin GameModule.dll basariyla yuklendi.");
            }

            result.success = true;
            result.log_messages.push_back("[ProjectExporter] Export basariyla tamamlandi!");
            return result;
        }
        catch (const std::exception& e) {
            result.error_message = std::string("Istisna: ") + e.what();
            return result;
        }
    }

} // namespace project

