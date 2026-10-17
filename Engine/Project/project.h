#pragma once

/**
 * @file project.h
 * @brief Oyun motoru proje tanımı, meta verisi ve proje yaşam döngüsü yönetim sınıfını tanımlar.
 */

#include <string>
#include <filesystem>
#include "../Renderer/render_settings.h"
#include "../Renderer/fog_settings.h"

struct EngineContext;

namespace project {

    /**
     * @struct project_descriptor
     * @brief Proje yapılandırma, grafik (rendering) ve ortam aydınlatma (lighting/fog) meta verilerini barındıran yapı (project.json).
     */
    struct project_descriptor {
        std::string name = "NewProject";
        std::string version = "1.0.0";
        std::string asset_directory = "Assets";
        std::string start_scene = "Assets/Scenes/Main.json";
        render_settings rendering;              ///< Projeye özel kaydedilen grafik boru hattı ayarları.
        fog_settings lighting;                  ///< Projeye özel kaydedilen ortam sis/aydınlatma ayarları.
    };

    /**
     * @class project_manager
     * @brief Proje oluşturma, açma, kaydetme ve sahne/varlık dizini senkronizasyonunu yöneten merkezi servis.
     */
    class project_manager {
    public:
        static project_manager& instance();

        /**
         * @brief Belirtilen dizinde yeni bir proje oluşturur ve ilklendirir.
         *
         * 1. Proje klasörünü ve Assets / Scenes alt dizinlerini oluşturur.
         * 2. Yalnızca bir ana kamera içeren başlangıç sahnesi (starter scene) üretir.
         * 3. Sahneyi Assets/Scenes/Main.json olarak kaydeder.
         * 4. project.json meta dosyasını (rendering ve lighting ayarlarıyla) kaydeder.
         * 5. Varlık Tarayıcısını (Asset Browser) yeni projenin Assets dizinine yönlendirir.
         *
         * @param parentFolder Projenin oluşturulacağı ana dizin.
         * @param projectName Projenin adı.
         * @param ctx Motor bağlamı.
         * @return true Proje başarıyla oluşturulduysa.
         */
        bool create_project(const std::string& parentFolder, const std::string& projectName, EngineContext& ctx);

        /**
         * @brief project.json dosyasını veya proje klasörünü okuyarak projeyi açar ve grafik/ışık ayarlarını uygular.
         *
         * @param projectPathOrJson project.json dosyasının veya proje kök klasörünün yolu.
         * @param ctx Motor bağlamı.
         * @return true Proje başarıyla yüklendiyse.
         */
        bool open_project(const std::string& projectPathOrJson, EngineContext& ctx);

        /**
         * @brief Aktif projenin project.json yapılandırma dosyasını motor bağlamındaki rendering ve lighting ayarlarıyla kaydeder.
         * @param ctx Motor bağlamı.
         */
        bool save_project(const EngineContext& ctx);

        /**
         * @brief Aktif projenin mevcut descriptor verilerini diske yazar.
         */
        bool save_project();

        /**
         * @brief Çalışma dizinindeki varsayılan proje yapısını tespit eder veya ilklendirir.
         */
        void init_default_project(EngineContext& ctx);

        /**
         * @brief Motor bağlamındaki renderSettings ve fog yapılandırmasını OpenGL donanım durumuna uygular.
         */
        static void apply_render_settings(EngineContext& ctx);

        /**
         * @brief Sahnede yalnızca bir kamera varlığı barındıran temiz bir başlangıç sahnesi kurar.
         */
        static void setup_starter_scene(EngineContext& ctx);

        [[nodiscard]] bool has_active_project() const noexcept { return m_hasActiveProject; }
        [[nodiscard]] const std::string& get_project_root() const noexcept { return m_projectRootPath; }
        [[nodiscard]] const std::string& get_project_name() const noexcept { return m_descriptor.name; }
        [[nodiscard]] const std::string& get_asset_path() const noexcept { return m_assetPath; }
        [[nodiscard]] const project_descriptor& get_descriptor() const noexcept { return m_descriptor; }
        [[nodiscard]] project_descriptor& get_descriptor() noexcept { return m_descriptor; }

    private:
        project_manager() = default;

        bool m_hasActiveProject = false;
        std::string m_projectRootPath;
        std::string m_projectFilePath;
        std::string m_assetPath;
        project_descriptor m_descriptor;
    };

} // namespace project

