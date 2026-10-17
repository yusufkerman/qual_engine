/**
 * @file project.cpp
 * @brief Proje oluşturma, project.json I/O, başlangıç sahnesi ve grafik/ışık ayarları kalıcılık uygulaması.
 */

#include "project.h"
#include "project_module.h"
#include "../Engine/engine_context.h"
#include "../ECS/components.h"
#include "../Sera/sera.h"
#include "../Externals/nlohmann/json.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>

namespace project {

    namespace {

        nlohmann::json render_settings_to_json(const render_settings& r)
        {
            nlohmann::json j;
            j["enable_frustum_culling"] = r.enable_frustum_culling;
            j["cull_bias"] = r.cull_bias;
            j["enable_shadows"] = r.enable_shadows;
            j["shadow_map_resolution"] = r.shadow_map_resolution;
            j["shadow_ortho_size"] = r.shadow_ortho_size;
            j["shadow_near_plane"] = r.shadow_near_plane;
            j["shadow_far_plane"] = r.shadow_far_plane;
            j["shadow_distance"] = r.shadow_distance;
            j["shadow_camera_height"] = r.shadow_camera_height;
            j["show_shadow_map_debug"] = r.show_shadow_map_debug;
            j["enable_skybox"] = r.enable_skybox;
            j["skybox_path"] = r.skybox_path;
            j["clear_color_scene"] = { r.clear_color_scene.r, r.clear_color_scene.g, r.clear_color_scene.b };
            j["clear_color_game"] = { r.clear_color_game.r, r.clear_color_game.g, r.clear_color_game.b };
            j["vsync"] = r.vsync;
            j["wireframe"] = r.wireframe;
            j["backface_culling"] = r.backface_culling;
            return j;
        }

        void render_settings_from_json(const nlohmann::json& j, render_settings& r)
        {
            if (j.contains("enable_frustum_culling")) r.enable_frustum_culling = j["enable_frustum_culling"].get<bool>();
            if (j.contains("cull_bias")) r.cull_bias = j["cull_bias"].get<float>();
            if (j.contains("enable_shadows")) r.enable_shadows = j["enable_shadows"].get<bool>();
            if (j.contains("shadow_map_resolution")) r.shadow_map_resolution = j["shadow_map_resolution"].get<int>();
            if (j.contains("shadow_ortho_size")) r.shadow_ortho_size = j["shadow_ortho_size"].get<float>();
            if (j.contains("shadow_near_plane")) r.shadow_near_plane = j["shadow_near_plane"].get<float>();
            if (j.contains("shadow_far_plane")) r.shadow_far_plane = j["shadow_far_plane"].get<float>();
            if (j.contains("shadow_distance")) r.shadow_distance = j["shadow_distance"].get<float>();
            if (j.contains("shadow_camera_height")) r.shadow_camera_height = j["shadow_camera_height"].get<float>();
            if (j.contains("show_shadow_map_debug")) r.show_shadow_map_debug = j["show_shadow_map_debug"].get<bool>();
            if (j.contains("enable_skybox")) r.enable_skybox = j["enable_skybox"].get<bool>();
            if (j.contains("skybox_path")) r.skybox_path = j["skybox_path"].get<std::string>();
            if (j.contains("clear_color_scene") && j["clear_color_scene"].is_array() && j["clear_color_scene"].size() >= 3)
            {
                r.clear_color_scene = glm::vec3(j["clear_color_scene"][0], j["clear_color_scene"][1], j["clear_color_scene"][2]);
            }
            if (j.contains("clear_color_game") && j["clear_color_game"].is_array() && j["clear_color_game"].size() >= 3)
            {
                r.clear_color_game = glm::vec3(j["clear_color_game"][0], j["clear_color_game"][1], j["clear_color_game"][2]);
            }
            if (j.contains("vsync")) r.vsync = j["vsync"].get<bool>();
            if (j.contains("wireframe")) r.wireframe = j["wireframe"].get<bool>();
            if (j.contains("backface_culling")) r.backface_culling = j["backface_culling"].get<bool>();
        }

        nlohmann::json fog_settings_to_json(const fog_settings& f)
        {
            nlohmann::json j;
            j["enabled"] = f.enabled;
            j["show_in_scene"] = f.show_in_scene;
            j["color"] = { f.color.r, f.color.g, f.color.b };
            j["density"] = f.density;
            return j;
        }

        void fog_settings_from_json(const nlohmann::json& j, fog_settings& f)
        {
            if (j.contains("enabled")) f.enabled = j["enabled"].get<bool>();
            if (j.contains("show_in_scene")) f.show_in_scene = j["show_in_scene"].get<bool>();
            if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3)
            {
                f.color = glm::vec3(j["color"][0], j["color"][1], j["color"][2]);
            }
            if (j.contains("density")) f.density = j["density"].get<float>();
        }

    } // anonymous namespace

    project_manager& project_manager::instance()
    {
        static project_manager s_instance;
        return s_instance;
    }

    void project_manager::apply_render_settings(EngineContext& ctx)
    {
        // 1. Dikey senkronizasyon (VSync)
        glfwSwapInterval(ctx.renderSettings.vsync ? 1 : 0);

        // 2. Tel kafes (Wireframe) çizim modu
        glPolygonMode(GL_FRONT_AND_BACK, ctx.renderSettings.wireframe ? GL_LINE : GL_FILL);

        // 3. Arka yüz kırpma (Backface Culling)
        if (ctx.renderSettings.backface_culling)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }

        // 4. Gölge çözünürlüğü değişmişse FBO'yu yeniden oluştur
        if (ctx.renderSettings.shadow_map_resolution > 0 &&
            static_cast<int>(ctx.shadowFBO.get_width()) != ctx.renderSettings.shadow_map_resolution)
        {
            ctx.shadowFBO.init(ctx.renderSettings.shadow_map_resolution, ctx.renderSettings.shadow_map_resolution);
        }
    }

    void project_manager::init_default_project(EngineContext& ctx)
    {
        const std::filesystem::path currentProjJson = std::filesystem::current_path() / "project.json";
        if (std::filesystem::exists(currentProjJson))
        {
            open_project(currentProjJson.string(), ctx);
        }
        else
        {
            m_descriptor.name = "OpenGL-Learn-1";
            m_descriptor.version = "1.0.0";
            m_descriptor.asset_directory = "Assets";
            m_descriptor.start_scene = "Assets/Scenes/Main.json";
            m_descriptor.rendering = ctx.renderSettings;
            m_descriptor.lighting = ctx.fog;

            m_hasActiveProject = true;
            m_projectRootPath = std::filesystem::current_path().string();
            m_projectFilePath = currentProjJson.string();
            m_assetPath = (std::filesystem::current_path() / "Assets").string();

            if (ctx.isEditorBuild)
            {
                ctx.vestaSys.set_asset_root(m_assetPath);
            }

            apply_render_settings(ctx);
            project_module::instance().unload_module();
            std::cout << "[ProjectManager] Varsayilan proje oturumu hazirlandi: " << m_projectRootPath << "\n";
        }
    }

    void project_manager::setup_starter_scene(EngineContext& ctx)
    {
        scene& sc = ctx.session.editor_scene();
        sc.new_scene();

        // Kullanıcı isteği doğrultusunda: Sahne yalnızca 1 adet kamera ile başlatılır.
        const entity camEntity = sc.create_entity();
        registry& reg = sc.get_registry();

        reg.add_component<name_component>(camEntity, "Main Camera");
        reg.add_component<transform_component>(
            camEntity,
            glm::vec3(0.0f, 2.0f, 5.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f)
        );
        reg.add_component<camera_component>(
            camEntity,
            60.0f,      // fov
            0.1f,       // nearPlane
            500.0f      // farPlane
        );

        sc.set_active_camera(camEntity);
        sc.update_light_batch();

        // Eski seçimi temizle
        if (ctx.isEditorBuild)
        {
            ctx.vestaSys.selection().clear_all();
        }

        std::cout << "[ProjectManager] Temiz baslangic sahnesi olusturuldu (Yalnizca kamera, Entity ID: " << camEntity << ").\n";
    }

    bool project_manager::create_project(const std::string& parentFolder, const std::string& projectName, EngineContext& ctx)
    {
        if (parentFolder.empty() || projectName.empty())
        {
            std::cerr << "[ProjectManager] Hata: Gecersiz proje yolu veya ismi.\n";
            return false;
        }

        try
        {
            const std::filesystem::path rootPath = std::filesystem::path(parentFolder) / projectName;
            const std::filesystem::path assetsPath = rootPath / "Assets";
            const std::filesystem::path scenesPath = assetsPath / "Scenes";

            // 1. Dizin hiyerarşisini oluştur
            std::filesystem::create_directories(scenesPath);

            // 2. Başlangıç sahnesini (yalnızca kamera) yapılandır ve kaydet
            setup_starter_scene(ctx);

            const std::filesystem::path mainSceneFile = scenesPath / "Main.json";
            if (ctx.sera)
            {
                ctx.sera->save_scene(mainSceneFile.string(), ctx.session.editor_scene());
            }

            // 3. project.json dosyasını hazırla ve kaydet
            m_descriptor.name = projectName;
            m_descriptor.version = "1.0.0";
            m_descriptor.asset_directory = "Assets";
            m_descriptor.start_scene = "Assets/Scenes/Main.json";
            m_descriptor.rendering = ctx.renderSettings;
            m_descriptor.lighting = ctx.fog;

            nlohmann::json j;
            j["name"] = m_descriptor.name;
            j["version"] = m_descriptor.version;
            j["asset_directory"] = m_descriptor.asset_directory;
            j["start_scene"] = m_descriptor.start_scene;
            j["rendering"] = render_settings_to_json(m_descriptor.rendering);
            j["lighting"] = fog_settings_to_json(m_descriptor.lighting);

            const std::filesystem::path projJsonPath = rootPath / "project.json";
            std::ofstream outFile(projJsonPath);
            if (!outFile.is_open())
            {
                std::cerr << "[ProjectManager] Hata: project.json olusturulamadi: " << projJsonPath << "\n";
                return false;
            }
            outFile << j.dump(4);
            outFile.close();

            // 4. Durum değişkenlerini güncelle
            m_hasActiveProject = true;
            m_projectRootPath = rootPath.string();
            m_projectFilePath = projJsonPath.string();
            m_assetPath = assetsPath.string();

            // 5. Varlık Tarayıcısını (Asset Browser) yeni projenin Assets dizinine yönlendir
            if (ctx.isEditorBuild)
            {
                ctx.vestaSys.set_asset_root(m_assetPath);
            }

            // 6. Pencere başlığını proje adı ile güncelle
            if (ctx.window)
            {
                const std::string title = "OpenGL-Learn-1 - [" + projectName + "]";
                glfwSetWindowTitle(ctx.window, title.c_str());
            }

            apply_render_settings(ctx);
            project_module::instance().unload_module();

            std::string scaffoldLog;
            project_module::ensure_script_scaffolding(m_projectRootPath, scaffoldLog);

            std::cout << "[ProjectManager] Proje basariyla olusturuldu: " << m_projectRootPath << "\n";
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ProjectManager] Proje olusturma sirasinda hata: " << e.what() << "\n";
            return false;
        }
    }

    bool project_manager::open_project(const std::string& projectPathOrJson, EngineContext& ctx)
    {
        if (projectPathOrJson.empty())
        {
            return false;
        }

        try
        {
            std::filesystem::path p(projectPathOrJson);
            std::filesystem::path projJsonPath;
            std::filesystem::path rootPath;

            if (std::filesystem::is_directory(p))
            {
                rootPath = p;
                projJsonPath = rootPath / "project.json";
            }
            else
            {
                projJsonPath = p;
                rootPath = p.parent_path();
            }

            if (!std::filesystem::exists(projJsonPath))
            {
                std::cerr << "[ProjectManager] Hata: project.json bulunamadi: " << projJsonPath << "\n";
                return false;
            }

            std::ifstream inFile(projJsonPath);
            if (!inFile.is_open())
            {
                std::cerr << "[ProjectManager] Hata: project.json acilamadi: " << projJsonPath << "\n";
                return false;
            }

            nlohmann::json j;
            inFile >> j;
            inFile.close();

            m_descriptor.name = j.value("name", rootPath.filename().string());
            m_descriptor.version = j.value("version", "1.0.0");
            m_descriptor.asset_directory = j.value("asset_directory", "Assets");
            m_descriptor.start_scene = j.value("start_scene", "Assets/Scenes/Main.json");

            // Grafik ve sis ayarlarını yükle
            if (j.contains("rendering"))
            {
                render_settings_from_json(j["rendering"], m_descriptor.rendering);
                ctx.renderSettings = m_descriptor.rendering;
            }
            if (j.contains("lighting"))
            {
                fog_settings_from_json(j["lighting"], m_descriptor.lighting);
                ctx.fog = m_descriptor.lighting;
            }

            m_hasActiveProject = true;
            m_projectRootPath = rootPath.string();
            m_projectFilePath = projJsonPath.string();
            m_assetPath = (rootPath / m_descriptor.asset_directory).string();

            // 1. Varlık Tarayıcısını proje Assets dizinine bağla
            if (ctx.isEditorBuild)
            {
                ctx.vestaSys.set_asset_root(m_assetPath);
                ctx.vestaSys.selection().clear_all();
            }

            // 2. Pencere başlığını güncelle
            if (ctx.window)
            {
                const std::string title = "OpenGL-Learn-1 - [" + m_descriptor.name + "]";
                glfwSetWindowTitle(ctx.window, title.c_str());
            }

            // 3. Render durumlarını OpenGL donanımına uygula
            apply_render_settings(ctx);

            // 4. Proje script iskeletini hazirla / tara
            std::string scaffoldLog;
            project_module::ensure_script_scaffolding(m_projectRootPath, scaffoldLog);

            // 5. Projeye ait dinamik GameModule.dll varsa sahneyi yuklemeden ONCE yukle ki betikler script_factory'de hazir olsun
            const std::filesystem::path gameModulePath = rootPath / "Binaries" / "GameModule.dll";
            if (std::filesystem::exists(gameModulePath))
            {
                project_module::instance().load_module(gameModulePath.string());
            }
            else
            {
                project_module::instance().unload_module();
            }

            // 6. Başlangıç sahnesini yükle (Betikler fabrikada hazir oldugu icin otomatik baglanir)
            const std::filesystem::path startSceneFile = rootPath / m_descriptor.start_scene;
            if (std::filesystem::exists(startSceneFile) && ctx.sera)
            {
                ctx.sera->load_scene(startSceneFile.string(), ctx.session.editor_scene());
            }
            else
            {
                // Sahne dosyası yoksa en azından bir kamera ile başlat
                setup_starter_scene(ctx);
            }

            std::cout << "[ProjectManager] Proje basariyla yuklendi: " << m_projectRootPath << " (" << m_descriptor.name << ")\n";
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ProjectManager] Proje yukleme sirasinda hata: " << e.what() << "\n";
            return false;
        }
    }

    bool project_manager::save_project(const EngineContext& ctx)
    {
        m_descriptor.rendering = ctx.renderSettings;
        m_descriptor.lighting = ctx.fog;
        return save_project();
    }

    bool project_manager::save_project()
    {
        if (!m_hasActiveProject || m_projectFilePath.empty())
        {
            return false;
        }

        try
        {
            nlohmann::json j;
            j["name"] = m_descriptor.name;
            j["version"] = m_descriptor.version;
            j["asset_directory"] = m_descriptor.asset_directory;
            j["start_scene"] = m_descriptor.start_scene;
            j["rendering"] = render_settings_to_json(m_descriptor.rendering);
            j["lighting"] = fog_settings_to_json(m_descriptor.lighting);

            std::ofstream outFile(m_projectFilePath);
            if (!outFile.is_open())
            {
                return false;
            }
            outFile << j.dump(4);
            outFile.close();

            std::cout << "[ProjectManager] Proje ayarlari (Rendering & Lighting) basariyla kaydedildi: " << m_projectFilePath << "\n";
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[ProjectManager] Proje kaydetme hatasi: " << e.what() << "\n";
            return false;
        }
    }

} // namespace project
