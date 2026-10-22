/**
 * @file vesta.cpp
 * @brief Vesta Editör Sisteminin başlatılmasını, panel kayıtlarını ve yaşam döngüsünü yönetir.
 */

#include "vesta.h"

 // --- Bileşen UI Kayıt Sistemleri ---
#include "Inspector/Components/camera_component_ui.h"
#include "Inspector/Components/collider_component_ui.h"
#include "Inspector/Components/light_component_ui.h"
#include "Inspector/Components/material_component_ui.h"
#include "Inspector/Components/mesh_component_ui.h"
#include "Inspector/Components/native_script_component_ui.h"
#include "Inspector/Components/rigidbody_component_ui.h"
#include "Inspector/Components/transform_component_ui.h"
#include "Inspector/Components/skinned_mesh_component_ui.h"
#include "Inspector/Components/animator_component_ui.h"
#include "Inspector/Components/skeleton_component_ui.h"
#include "Inspector/Components/bone_attachment_component_ui.h"
#include "Inspector/Components/ui_components_ui.h"
#include "Inspector/Assets/material_inspector.h"
#include "Inspector/Assets/texture_inspector.h"
#include "Inspector/Assets/audio_clip_inspector.h"
#include "Inspector/Components/audio_component_ui.h"

// --- Çekirdek Sistemler ve Fizik ---
#include "CoreState/vesta_module.h" 
#include "../../Systems/physics_system.h"
#include "../../Systems/physics_api.h"    
#include "../../Sera/sera.h"
#include "../Project/project.h"
#include "../Project/project_module.h"
#include "../Project/project_exporter.h"
#include "Widgets/loading_bar_widget.h"
#include "../Project/engine_settings.h"
#include "../../Audio/audio_system.h"
#include <GLFW/glfw3.h>
#include "../../Async/async_task_manager.h"
#include "../../Externals/nfd/nfd.h"
#include <imgui.h>

#include <iostream>

namespace vesta {

    void vesta_system::init(aria::system* ariaSys, EngineContext& engineCtx, EditorContext& editorCtx)
    {
        std::cout << "[Vesta] Editor alt sistemleri baslatiliyor...\n";

        // Güvenli yeniden başlatma (re-init) için panelleri temizle
        m_panels.clear();

        // 1. Bileşen (Component) Arayüz Kayıtları
        register_transform_ui_system();
        register_camera_ui_system();
        register_light_ui_system();
        register_rigidbody_ui_system();
        register_collider_ui_system();
        register_native_script_ui_system();
        register_material_ui_system(*engineCtx.aria_system);
        register_mesh_ui_system(*engineCtx.aria_system);
        register_skinned_mesh_ui_system(*engineCtx.aria_system);
        register_animator_ui_system(*engineCtx.aria_system);
        register_skeleton_ui_system(*engineCtx.aria_system);
        register_bone_attachment_ui_system();
        register_canvas_ui_system();
        register_rect_transform_ui_system();
        register_ui_text_ui_system();
        register_ui_image_ui_system(engineCtx.aria_system);
        register_ui_button_ui_system();
        register_audio_components_ui_system(*engineCtx.aria_system, engineCtx.audioSystem);

        // 2. Çekirdek Servislerin Hazırlanması
        registry& reg = engineCtx.session.game_scene().get_registry();

        m_editable_world = std::make_unique<editable_world_impl>(reg, ariaSys);
        m_command_stack = std::make_unique<command_stack>(*m_editable_world);
        m_material_service = std::make_unique<material_service>(*m_editable_world, *m_command_stack);
        m_selection_manager = std::make_unique<editor_selection_manager>();

        // 3. Varlık (Asset) Arayüz Kayıtları
        register_material_asset_ui_system(engineCtx.aria_system, m_material_service.get());
        register_texture_asset_ui_system(engineCtx.aria_system);
        register_audio_clip_asset_ui_system(engineCtx.aria_system, &engineCtx.audioSystem);

        // 4. Editör Panelleri ve ViewModel'lerin Başlatılması
        init_top_bar(engineCtx, editorCtx);
        init_scene_viewport(engineCtx, editorCtx);
        init_game_viewport(engineCtx);
        init_hierarchy(reg, engineCtx);
        init_inspector(reg, engineCtx);
        init_asset_browser(*ariaSys);
        init_lighting(engineCtx);
        init_rendering_settings(engineCtx);

        // Top bar Window menüsü için panel listesi referansını ata
        if (m_top_bar_panel)
        {
            m_top_bar_panel->set_panels_reference(&m_panels);
        }

        // 5. Fizik Kancalarının (Hooks) Uygulanması
        auto& physApi = engineCtx.physicsSystem.api();
        apply_hooks(physApi);
    }

    void vesta_system::register_panel(IEditorPanel* panel)
    {
        if (panel) {
            m_panels.push_back(panel);
        }
    }

    void vesta_system::apply_hooks(physics_api& api)
    {
        // Rigidbody silindiğinde fizik dünyasından da temizlenmesi için kanca eklendi
        vesta::vesta_module::register_remove_hook<rigidbody_component>(
            [&api](entity e)
            {
                api.destroy_body(e);
            });

        // Collider silindiğinde fizik dünyasından şeklinin kaldırılması için kanca eklendi
        vesta::vesta_module::register_remove_hook<collider_component>(
            [&api](entity e)
            {
                api.remove_collider_shape(e);
            });
    }

    void vesta_system::init_top_bar(EngineContext& engineCtx, EditorContext& editorCtx)
    {
        m_top_bar_vm = std::make_unique<top_bar_view_model>(
            editorCtx,
            [&engineCtx](const std::string& path)
            {
                if (engineCtx.sera && !path.empty())
                {
                    const std::string fileName = std::filesystem::path(path).filename().string();
                    core::async_task_manager::instance().run_task(
                        "Saving Scene (" + fileName + ")",
                        [&engineCtx, path](core::task_reporter& reporter) -> bool {
                            reporter.set_indeterminate("Writing scene entities & components to disk...");
                            engineCtx.sera->save_scene(path, engineCtx.session.editor_scene());
                            return true;
                        }
                    );
                }
            },
            [&engineCtx](const std::string& path)
            {
                if (engineCtx.sera && !path.empty())
                {
                    const std::string fileName = std::filesystem::path(path).filename().string();
                    core::async_task_manager::instance().run_task(
                        "Loading Scene (" + fileName + ")",
                        [&engineCtx, path](core::task_reporter& reporter) -> bool {
                            reporter.set_indeterminate("Reading scene data from disk...");
                            return true;
                        },
                        [&engineCtx, path](bool success, const std::string& message) {
                            try {
                                engineCtx.sera->load_scene(path, engineCtx.session.editor_scene());
                            }
                            catch (const std::exception& e) {
                                std::cerr << "[Vesta] Error loading scene: " << e.what() << "\n";
                            }
                        }
                    );
                }
            }
        );

        m_top_bar_panel = std::make_unique<top_bar_panel>(*m_top_bar_vm);
        register_panel(m_top_bar_panel.get());
    }

    void vesta_system::init_scene_viewport(EngineContext& engineCtx, EditorContext& editorCtx)
    {
        m_scene_viewport_vm = std::make_unique<scene_viewport_view_model>(m_viewport_state);

        if (m_scene_viewport_vm) {
            m_scene_viewport_vm->set_texture_id(engineCtx.editorTarget.tex);
        }

        m_scene_viewport_panel = std::make_unique<scene_viewport_panel>(
            *m_scene_viewport_vm,
            m_viewport_state,
            &engineCtx,
            &editorCtx
        );
        register_panel(m_scene_viewport_panel.get());
    }

    void vesta_system::init_game_viewport(EngineContext& ctx)
    {
        m_game_viewport_vm = std::make_unique<game_viewport_view_model>(m_viewport_state);

        if (m_game_viewport_vm) {
            m_game_viewport_vm->set_texture_id(ctx.gameTarget.tex);
        }

        m_game_viewport_panel = std::make_unique<game_viewport_panel>(*m_game_viewport_vm, m_viewport_state, &ctx);
        register_panel(m_game_viewport_panel.get());
    }

    void vesta_system::init_hierarchy(registry& registry, EngineContext& engineCtx)
    {
        m_hierarchy_panel = std::make_unique<hierarchy_panel>(
            *m_editable_world,
            registry,
            *m_selection_manager,
            &engineCtx.gizmoSystem.settings()
        );

        register_panel(m_hierarchy_panel.get());
    }

    void vesta_system::init_inspector(registry& reg, EngineContext& engineCtx)
    {
        m_inspector_vm = std::make_unique<inspector_view_model>(
            *m_editable_world,
            reg,
            *m_selection_manager
        );

        m_inspector_panel = std::make_unique<inspector_panel>(
            std::move(m_inspector_vm),
            &engineCtx.gizmoSystem.settings()
        );
        register_panel(m_inspector_panel.get());
    }

    void vesta_system::init_asset_browser(aria::system& ariaSys)
    {
        m_asset_browser_system = std::make_unique<asset_browser_system>(
            "Assets",
            &ariaSys,
            m_material_service.get()
        );

        m_asset_browser_system->refresh();

        m_asset_browser_vm = std::make_unique<asset_browser_view_model>(
            *m_asset_browser_system,
            *m_material_service,
            [this](const std::string& path)
            {
                if (path.empty()) {
                    m_selection_manager->clear_asset_selection();
                }
                else {
                    m_selection_manager->select_asset(path);
                }
            },
            [this]()
            {
                m_selection_manager->clear_entity_selection();
            }
        );

        m_asset_browser_panel = std::make_unique<asset_browser_panel>(*m_asset_browser_vm);
        m_asset_browser_panel->init();

        register_panel(m_asset_browser_panel.get());
    }

    void vesta_system::init_lighting(EngineContext& engineCtx)
    {
        m_lighting_panel = std::make_unique<lighting_panel>(engineCtx);
        register_panel(m_lighting_panel.get());
    }

    void vesta_system::init_rendering_settings(EngineContext& engineCtx)
    {
        m_rendering_panel = std::make_unique<rendering_panel>(engineCtx);
        register_panel(m_rendering_panel.get());
    }

    void vesta_system::set_asset_root(const std::string& newPath)
    {
        if (m_asset_browser_system)
        {
            m_asset_browser_system->set_root_path(newPath);
        }
        if (m_asset_browser_vm)
        {
            m_asset_browser_vm->refresh();
        }
    }

    IEditorPanel* vesta_system::find_panel(const std::string& title) const
    {
        for (auto* p : m_panels)
        {
            if (p && p->get_title() == title)
            {
                return p;
            }
        }
        return nullptr;
    }

    void vesta_system::render_main_menu_bar(EngineContext& engineCtx, EditorContext& editorCtx)
    {
        static bool s_showNewProjectModal = false;
        static char s_projectNameBuf[128] = "MyGame";
        static char s_projectParentFolderBuf[512] = "";

        static bool s_showExportModal = false;
        static char s_exportFolderBuf[512] = "";
        static bool s_exportCompileScripts = true;
        static bool s_exportEngineResources = true;
        static bool s_exportDependencies = true;
        static bool s_exportFullscreen = false;
        static std::string s_exportStatusMsg = "";
        static bool s_exportSuccess = false;

        static bool s_showScriptStatusModal = false;
        static bool s_scriptStatusSuccess = false;
        static std::string s_scriptStatusMsg = "";
        static bool s_showEngineSettingsModal = false;

        if (s_exportFolderBuf[0] == '\0')
        {
            const std::string defaultExport = (std::filesystem::current_path() / "Builds").string();
            snprintf(s_exportFolderBuf, sizeof(s_exportFolderBuf), "%s", defaultExport.c_str());
        }

        if (s_projectParentFolderBuf[0] == '\0')
        {
            const std::string currentDir = std::filesystem::current_path().string();
            snprintf(s_projectParentFolderBuf, sizeof(s_projectParentFolderBuf), "%s", currentDir.c_str());
        }

        // ========================================================================
        // 1. FILE MENÜSÜ (PROJE VE SAHNE YÖNETİMİ)
        // ========================================================================
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project...", "Ctrl+N"))
            {
                s_showNewProjectModal = true;
            }

            if (ImGui::MenuItem("Open Project...", "Ctrl+O"))
            {
                nfdu8char_t* outPath = nullptr;
                nfdu8filteritem_t filterItem[1] = { { "Project File", "json" } };

                if (NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr) == NFD_OKAY)
                {
                    if (outPath != nullptr)
                    {
                        const std::string projPath = outPath;
                        const std::string projName = std::filesystem::path(projPath).filename().string();
                        core::async_task_manager::instance().run_task(
                            "Opening Project (" + projName + ")",
                            [](core::task_reporter& reporter) -> bool {
                                reporter.set_indeterminate("Parsing project file & starter scene...");
                                return true;
                            },
                            [&engineCtx, projPath](bool success, const std::string& msg) {
                                try
                                {
                                    project::project_manager::instance().open_project(projPath, engineCtx);
                                }
                                catch (const std::exception& ex)
                                {
                                    std::cerr << "[Vesta] Error opening project: " << ex.what() << "\n";
                                }
                                catch (...)
                                {
                                    std::cerr << "[Vesta] Unknown error occurred while opening project.\n";
                                }
                            }
                        );
                        NFD_FreePathU8(outPath);
                    }
                }
            }

            const bool hasActiveProj = project::project_manager::instance().has_active_project();
            if (ImGui::MenuItem("Save Project", "Ctrl+Shift+S", false, hasActiveProj && !core::async_task_manager::instance().is_busy()))
            {
                core::async_task_manager::instance().run_task(
                    "Saving Project",
                    [&engineCtx](core::task_reporter& reporter) -> bool {
                        reporter.set_indeterminate("Saving project descriptor & current scene...");
                        project::project_manager::instance().save_project(engineCtx);
                        if (engineCtx.sera && !project::project_manager::instance().get_descriptor().start_scene.empty())
                        {
                            const std::string scenePath = project::project_manager::instance().get_project_root() + "/" +
                                project::project_manager::instance().get_descriptor().start_scene;
                            engineCtx.sera->save_scene(scenePath, engineCtx.session.editor_scene());
                        }
                        return true;
                    }
                );
            }

            ImGui::Separator();

            const bool isPlaying = (editorCtx.mode == EngineMode::PLAY);

            if (ImGui::MenuItem("New Scene", nullptr, false, !isPlaying))
            {
                project::project_manager::setup_starter_scene(engineCtx);
            }

            if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, !isPlaying && !core::async_task_manager::instance().is_busy()))
            {
                nfdu8char_t* outPath = nullptr;
                nfdu8filteritem_t filterItem[1] = { { "Engine Scene", "json" } };

                if (NFD_SaveDialogU8(&outPath, filterItem, 1, nullptr, "scene.json") == NFD_OKAY)
                {
                    if (outPath != nullptr)
                    {
                        if (engineCtx.sera)
                        {
                            const std::string filePath = outPath;
                            const std::string fileName = std::filesystem::path(filePath).filename().string();
                            core::async_task_manager::instance().run_task(
                                "Saving Scene (" + fileName + ")",
                                [&engineCtx, filePath](core::task_reporter& reporter) -> bool {
                                    reporter.set_indeterminate("Writing scene entities & components to disk...");
                                    engineCtx.sera->save_scene(filePath, engineCtx.session.editor_scene());
                                    return true;
                                }
                            );
                        }
                        NFD_FreePathU8(outPath);
                    }
                }
            }

            if (ImGui::MenuItem("Load Scene...", nullptr, false, !isPlaying && !core::async_task_manager::instance().is_busy()))
            {
                nfdu8char_t* outPath = nullptr;
                nfdu8filteritem_t filterItem[1] = { { "Engine Scene", "json" } };

                if (NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr) == NFD_OKAY)
                {
                    if (outPath != nullptr)
                    {
                        if (engineCtx.sera)
                        {
                            const std::string filePath = outPath;
                            const std::string fileName = std::filesystem::path(filePath).filename().string();
                            core::async_task_manager::instance().run_task(
                                "Loading Scene (" + fileName + ")",
                                [&engineCtx, filePath](core::task_reporter& reporter) -> bool {
                                    reporter.set_indeterminate("Reading scene data from disk...");
                                    return true;
                                },
                                [&engineCtx, filePath](bool success, const std::string& message) {
                                    try
                                    {
                                        engineCtx.sera->load_scene(filePath, engineCtx.session.editor_scene());
                                    }
                                    catch (const std::exception& e)
                                    {
                                        std::cerr << "[Vesta] Error loading scene: " << e.what() << "\n";
                                    }
                                    catch (...)
                                    {
                                        std::cerr << "[Vesta] Unknown error occurred while loading scene.\n";
                                    }
                                }
                            );
                        }
                        NFD_FreePathU8(outPath);
                    }
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                if (engineCtx.window)
                {
                    glfwSetWindowShouldClose(engineCtx.window, GLFW_TRUE);
                }
            }

            ImGui::EndMenu();
        }

        // ========================================================================
        // 2. PROJECT MENU (SCRIPT BUILD & STANDALONE EXPORT)
        // ========================================================================
        const bool hasActiveProj = project::project_manager::instance().has_active_project();
        const bool isBusy = core::async_task_manager::instance().is_busy();

        static std::filesystem::file_time_type s_lastScriptBuildTime = std::filesystem::file_time_type::min();

        auto get_max_script_timestamp = [](const std::string& projectRoot) -> std::filesystem::file_time_type {
            namespace fs = std::filesystem;
            fs::file_time_type maxTime = fs::file_time_type::min();
            try {
                const fs::path root(projectRoot);
                const std::vector<fs::path> searchDirs = {
                    root / "Assets" / "Scripts",
                    root / "Scripts",
                    root / "Assets"
                };
                for (const auto& dir : searchDirs) {
                    if (!fs::exists(dir)) continue;
                    for (const auto& entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied)) {
                        if (!entry.is_regular_file()) continue;
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
                            if (entry.path().filename() == "GameModule.cpp") continue;
                            auto lwt = entry.last_write_time();
                            if (lwt > maxTime) {
                                maxTime = lwt;
                            }
                        }
                    }
                }
            } catch (...) {}
            return maxTime;
        };

        auto triggerBuildScripts = [&engineCtx, hasActiveProj, get_max_script_timestamp](bool showModal = true) {
            if (!hasActiveProj || core::async_task_manager::instance().is_busy()) return;
            const std::string root = project::project_manager::instance().get_project_root();
            project::project_module::instance().unload_module();

            core::async_task_manager::instance().run_task(
                "Compiling Project Scripts (F7)",
                [root](core::task_reporter& reporter) -> bool {
                    reporter.set_indeterminate("Scanning scripts & compiling GameModule.dll...");
                    std::string log;
                    bool ok = project::project_module::compile_project_scripts(root, log);
                    if (!ok) {
                        throw std::runtime_error(log.empty() ? "Script compilation failed." : log);
                    }
                    return true;
                },
                [root, showModal, get_max_script_timestamp](bool success, const std::string& message) {
                    s_lastScriptBuildTime = get_max_script_timestamp(root);
                    if (showModal || !success) {
                        s_showScriptStatusModal = true;
                    }
                    if (success)
                    {
                        std::filesystem::path dllPath = std::filesystem::path(root) / "Binaries" / "GameModule.dll";
                        bool loaded = project::project_module::instance().load_module(dllPath.string());
                        s_scriptStatusSuccess = loaded;
                        if (loaded)
                        {
                            const auto& scripts = project::project_module::instance().get_loaded_scripts();
                            s_scriptStatusMsg = "Project scripts compiled and loaded successfully!\n\nRegistered (" + 
                                                std::to_string(scripts.size()) + ") script class(es):\n";
                            for (const auto& s : scripts)
                            {
                                s_scriptStatusMsg += "  * " + s + "\n";
                            }
                        }
                        else
                        {
                            s_scriptStatusMsg = "Build succeeded but failed to load GameModule.dll:\n" + dllPath.string();
                        }
                    }
                    else
                    {
                        s_scriptStatusSuccess = false;
                        s_scriptStatusMsg = "Failed to compile project scripts:\n\n" + message;
                    }
                }
            );
        };

        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("Build Project Scripts", "F7", false, hasActiveProj && !isBusy))
            {
                triggerBuildScripts(true);
            }

            if (ImGui::MenuItem("Reload Game Module", "Ctrl+R", false, hasActiveProj && !isBusy))
            {
                project::project_module::instance().reload_module();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Export Standalone Game...", "Ctrl+E", false, hasActiveProj && !isBusy))
            {
                s_showExportModal = true;
                s_exportStatusMsg = "";
            }

            ImGui::EndMenu();
        }

        // F7 shortcut
        if (ImGui::IsKeyPressed(ImGuiKey_F7) && !ImGui::GetIO().WantTextInput && hasActiveProj && !isBusy)
        {
            triggerBuildScripts(true);
        }

        static bool s_wasWindowFocused = true;
        if (engineCtx.window)
        {
            const bool isFocused = (glfwGetWindowAttrib(engineCtx.window, GLFW_FOCUSED) != 0);
            if (isFocused && !s_wasWindowFocused)
            {
                if (project::engine_settings::instance().get_auto_reload_on_focus() &&
                    hasActiveProj && !isBusy && editorCtx.mode == EngineMode::EDITOR)
                {
                    const std::string root = project::project_manager::instance().get_project_root();
                    const auto currentMax = get_max_script_timestamp(root);
                    if (s_lastScriptBuildTime == std::filesystem::file_time_type::min())
                    {
                        s_lastScriptBuildTime = currentMax;
                    }
                    else if (currentMax > s_lastScriptBuildTime)
                    {
                        std::cout << "[HotReload] Script files modified. Rebuilding project scripts...\n";
                        triggerBuildScripts(false);
                    }
                }
            }
            s_wasWindowFocused = isFocused;
        }


        // ========================================================================
        // 3. WINDOW MENÜSÜ (GENERAL VE RENDERING PENCERELERİ)
        // ========================================================================
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::BeginMenu("General"))
            {
                auto drawPanelItem = [this](const char* title) {
                    IEditorPanel* p = find_panel(title);
                    if (!p) return;
                    const bool isOpen = p->is_open();
                    if (ImGui::MenuItem(title, nullptr, isOpen, !isOpen))
                    {
                        p->set_open(true);
                    }
                };

                drawPanelItem("Scene");
                drawPanelItem("Game");
                drawPanelItem("Hierarchy");
                drawPanelItem("Inspector");
                drawPanelItem("Asset Browser");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Rendering"))
            {
                auto drawPanelItem = [this](const char* title) {
                    IEditorPanel* p = find_panel(title);
                    if (!p) return;
                    const bool isOpen = p->is_open();
                    if (ImGui::MenuItem(title, nullptr, isOpen, !isOpen))
                    {
                        p->set_open(true);
                    }
                };

                drawPanelItem("Rendering Settings");
                drawPanelItem("Lighting");

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Reset Layout"))
            {
                engineCtx.uiManager.resetLayout();
            }

            ImGui::EndMenu();
        }
        // ========================================================================
        // 4. OPTIONS MENUSU (ENGINE PREFERENCES & SETTINGS)
        // ========================================================================
        if (ImGui::BeginMenu("Options"))
        {
            if (ImGui::MenuItem("Engine Settings..."))
            {
                s_showEngineSettingsModal = true;
            }
            ImGui::EndMenu();
        }

        // ========================================================================
        // 4. KÜRESEL SAĞ ÜST MODERN ASENKRON DURUM GÖSTERGESİ (CORNER LOADING BAR)
        // ========================================================================
        if (core::async_task_manager::instance().is_busy())
        {
            constexpr float kCornerBarWidth = 260.0f;
            constexpr float kCornerBarHeight = 18.0f;
            const float menuBarWidth = ImGui::GetWindowWidth();
            const float currentX = ImGui::GetCursorPosX();
            const float targetX = menuBarWidth - kCornerBarWidth - 10.0f;

            if (targetX > currentX + 5.0f)
            {
                ImGui::SameLine(targetX);
            }
            else
            {
                ImGui::SameLine(0.0f, 10.0f);
            }

            render_loading_bar_widget(kCornerBarWidth, kCornerBarHeight);
        }

        // ========================================================================
        // 3. MODAL POPUP: YENİ PROJE OLUŞTURMA
        // ========================================================================
        if (s_showNewProjectModal)
        {
            ImGui::OpenPopup("New Project Dialog");
            s_showNewProjectModal = false;
        }

        if (ImGui::BeginPopupModal("New Project Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Create New Game Project");
            ImGui::TextDisabled("Initialize project directory, assets, and starter scene with only a camera.");
            ImGui::Separator();

            ImGui::Text("Project Name:");
            ImGui::InputText("##NewProjectName", s_projectNameBuf, sizeof(s_projectNameBuf));

            ImGui::Spacing();
            ImGui::Text("Location:");
            ImGui::InputText("##NewProjectLocation", s_projectParentFolderBuf, sizeof(s_projectParentFolderBuf));
            ImGui::SameLine();
            if (ImGui::Button("Browse..."))
            {
                nfdu8char_t* outFolder = nullptr;
                if (NFD_PickFolderU8(&outFolder, s_projectParentFolderBuf) == NFD_OKAY)
                {
                    if (outFolder != nullptr)
                    {
                        snprintf(s_projectParentFolderBuf, sizeof(s_projectParentFolderBuf), "%s", outFolder);
                        NFD_FreePathU8(outFolder);
                    }
                }
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Will create: %s/%s/Assets/ (with starter camera scene)", s_projectParentFolderBuf, s_projectNameBuf);
            ImGui::Separator();

            if (ImGui::Button("Create Project", ImVec2(140, 0)))
            {
                if (strlen(s_projectNameBuf) > 0 && strlen(s_projectParentFolderBuf) > 0)
                {
                    project::project_manager::instance().create_project(s_projectParentFolderBuf, s_projectNameBuf, engineCtx);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // 4. MODAL POPUP: STANDALONE OYUN DIŞA AKTARMA (EXPORT)
        // ========================================================================
        if (s_showExportModal)
        {
            ImGui::OpenPopup("Export Standalone Game");
            s_showExportModal = false;
        }

        if (ImGui::BeginPopupModal("Export Standalone Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Export Standalone Game (Executable)");
            ImGui::TextDisabled("Bundles executable player, GameModule.dll, assets, shaders, and boot.config.");
            ImGui::Separator();

            ImGui::Text("Destination Directory:");
            ImGui::InputText("##ExportLocation", s_exportFolderBuf, sizeof(s_exportFolderBuf));
            ImGui::SameLine();
            if (ImGui::Button("Browse...##Export"))
            {
                nfdu8char_t* outFolder = nullptr;
                if (NFD_PickFolderU8(&outFolder, s_exportFolderBuf) == NFD_OKAY)
                {
                    if (outFolder != nullptr)
                    {
                        snprintf(s_exportFolderBuf, sizeof(s_exportFolderBuf), "%s", outFolder);
                        NFD_FreePathU8(outFolder);
                    }
                }
            }

            ImGui::Spacing();
            ImGui::Checkbox("Recompile Scripts (GameModule.dll)", &s_exportCompileScripts);
            ImGui::Checkbox("Bundle Engine Resources (EngineResources/)", &s_exportEngineResources);
            ImGui::Checkbox("Copy Dependencies (assimp-vc145-mt.dll)", &s_exportDependencies);
            ImGui::Checkbox("Start in Fullscreen", &s_exportFullscreen);

            ImGui::Spacing();
            ImGui::TextDisabled("Resolution: 1280 x 720 (Defined in boot.config)");

            if (!s_exportStatusMsg.empty())
            {
                ImGui::Spacing();
                ImGui::Separator();
                if (s_exportSuccess)
                    ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.52f, 1.0f), "%s", s_exportStatusMsg.c_str());
                else
                    ImGui::TextColored(ImVec4(0.92f, 0.35f, 0.35f, 1.0f), "%s", s_exportStatusMsg.c_str());
            }

            ImGui::Separator();

            const bool isBusyExport = core::async_task_manager::instance().is_busy();
            if (ImGui::Button("Package & Export", ImVec2(160, 0)) && !isBusyExport)
            {
                project::export_options opt;
                opt.output_directory = s_exportFolderBuf;
                opt.compile_scripts = s_exportCompileScripts;
                opt.copy_engine_resources = s_exportEngineResources;
                opt.copy_dependencies = s_exportDependencies;
                opt.fullscreen = s_exportFullscreen;
                opt.screen_width = 1280;
                opt.screen_height = 720;

                s_exportStatusMsg = "Packaging project and exporting standalone game in background...";
                s_exportSuccess = true;

                core::async_task_manager::instance().run_task(
                    "Exporting Standalone Game",
                    [opt](core::task_reporter& reporter) -> bool {
                        reporter.set_indeterminate("Compiling standalone runtime & bundling assets...");
                        auto res = project::project_exporter::export_project(opt);
                        if (!res.success) {
                            throw std::runtime_error(res.error_message.empty() ? "Export failed." : res.error_message);
                        }
                        return true;
                    },
                    [](bool success, const std::string& message) {
                        s_exportSuccess = success;
                        if (success) {
                            s_exportStatusMsg = "Project exported successfully!";
                        } else {
                            s_exportStatusMsg = "Export failed:\n" + message;
                        }
                    }
                );
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2(100, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // 5. MODAL POPUP: BETIK DERLEME DURUMU (SCRIPT BUILD STATUS)
        // ========================================================================
        if (s_showScriptStatusModal)
        {
            ImGui::OpenPopup("Project Scripts Status");
            s_showScriptStatusModal = false;
        }

        if (ImGui::BeginPopupModal("Project Scripts Status", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (s_scriptStatusSuccess)
            {
                ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.52f, 1.0f), "Scripts Built & Loaded Successfully!");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.92f, 0.35f, 0.35f, 1.0f), "Script Build / Load Failed!");
            }
            ImGui::Separator();

            ImGui::BeginChild("ScriptStatusScroll", ImVec2(520, 220), true);
            ImGui::TextUnformatted(s_scriptStatusMsg.c_str());
            ImGui::EndChild();

            ImGui::Separator();
            if (ImGui::Button("Copy to Clipboard"))
            {
                ImGui::SetClipboardText(s_scriptStatusMsg.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2(100, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        // ========================================================================
        // 6. MODAL POPUP: ENGINE SETTINGS / OPTIONS
        // ========================================================================
        if (s_showEngineSettingsModal)
        {
            ImGui::OpenPopup("Engine Settings");
            s_showEngineSettingsModal = false;
        }

        if (ImGui::BeginPopupModal("Engine Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "QualEngine Preferences & Settings");
            ImGui::Separator();

            auto& settings = project::engine_settings::instance();
            project::code_editor_type currentEditor = settings.get_editor_type();

            ImGui::Text("External Script & Code Editor:");
            int editorIdx = static_cast<int>(currentEditor);
            if (ImGui::RadioButton("Visual Studio Code (Auto-Detect)", &editorIdx, 0)) {
                settings.set_editor_type(project::code_editor_type::vs_code);
            }
            if (ImGui::RadioButton("Visual Studio (IDE)", &editorIdx, 1)) {
                settings.set_editor_type(project::code_editor_type::visual_studio);
            }
            if (ImGui::RadioButton("Custom External Editor", &editorIdx, 2)) {
                settings.set_editor_type(project::code_editor_type::custom);
            }

            if (settings.get_editor_type() == project::code_editor_type::vs_code)
            {
                std::string vsPath = settings.detect_vscode();
                if (!vsPath.empty()) {
                    ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.52f, 1.0f), "  [Found] %s", vsPath.c_str());
                } else {
                    ImGui::TextColored(ImVec4(0.92f, 0.35f, 0.35f, 1.0f), "  [!] VS Code not detected in standard paths. Fallback to system PATH 'code'.");
                }
            }
            else if (settings.get_editor_type() == project::code_editor_type::custom)
            {
                static char customPathBuf[512] = "";
                static bool bufInit = false;
                if (!bufInit) {
                    snprintf(customPathBuf, sizeof(customPathBuf), "%s", settings.get_custom_editor_path().c_str());
                    bufInit = true;
                }
                ImGui::Text("Executable Path:");
                if (ImGui::InputText("##CustomEditorPath", customPathBuf, sizeof(customPathBuf))) {
                    settings.set_custom_editor_path(customPathBuf);
                }
                ImGui::SameLine();
                if (ImGui::Button("Browse...##CustomEditor")) {
                    nfdchar_t* outPath = nullptr;
                    nfdfilteritem_t filterItem[1] = { { "Executable", "exe" } };
                    if (NFD_OpenDialog(&outPath, filterItem, 1, nullptr) == NFD_OKAY) {
                        if (outPath != nullptr) {
                            snprintf(customPathBuf, sizeof(customPathBuf), "%s", outPath);
                            settings.set_custom_editor_path(customPathBuf);
                            NFD_FreePath(outPath);
                        }
                    }
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Scripting & Hot-Reload:");
            bool autoReload = settings.get_auto_reload_on_focus();
            if (ImGui::Checkbox("Auto-recompile / reload on window focus", &autoReload)) {
                settings.set_auto_reload_on_focus(autoReload);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Audio System Master Settings:");
            {
                float masterVol = engineCtx.audioSystem.get_master_volume();
                if (ImGui::SliderFloat("Master Volume", &masterVol, 0.0f, 1.0f, "%.2f")) {
                    engineCtx.audioSystem.set_master_volume(masterVol);
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Save & Close", ImVec2(120, 0)))
            {
                settings.save();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                settings.load();
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void vesta_system::shutdown()
    {
        m_panels.clear();
        m_asset_browser_panel.reset();
        m_asset_browser_vm.reset();
        m_asset_browser_system.reset();
        m_material_service.reset();
        m_inspector_panel.reset();
        m_inspector_vm.reset();
        m_hierarchy_panel.reset();
        m_game_viewport_panel.reset();
        m_game_viewport_vm.reset();
        m_scene_viewport_panel.reset();
        m_scene_viewport_vm.reset();
        m_top_bar_panel.reset();
        m_top_bar_vm.reset();
        m_lighting_panel.reset();
        m_rendering_panel.reset();
        m_command_stack.reset();
        m_editable_world.reset();
        m_selection_manager.reset();
    }

    void vesta_system::update()
    {
        for (auto* panel : m_panels)
        {
            if (panel && panel->is_open())
            {
                panel->render();
            }
        }
    }

} // namespace vesta
