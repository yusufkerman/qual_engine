#pragma once

/**
 * @file game_runtime.h
 * @brief Standalone (Editörsüz, bağımsız) oyun yürütücüsü (Player) yapılandırma ve yaşam döngüsü arayüzü.
 */

#include <string>

struct EngineContext;

namespace runtime {

    /**
     * @struct boot_config
     * @brief Dağıtılan bağımsız oyunun başlangıç ayarlarını (boot.config) barındırır.
     */
    struct boot_config {
        std::string project_name = "StandaloneGame";
        std::string start_scene = "Assets/Scenes/Main.json";
        std::string asset_directory = "Assets";
        int screen_width = 1280;
        int screen_height = 720;
        bool fullscreen = false;
        bool vsync = true;
    };

    /**
     * @class game_runtime
     * @brief Editör panellerinden tamamen arındırılmış, doğrudan oyun sahnesini yükleyip oynatan yürütücü.
     */
    class game_runtime {
    public:
        /**
         * @brief boot.config dosyasını okuyarak yapılandırma nesnesini döndürür.
         */
        [[nodiscard]] static boot_config load_boot_config(const std::string& config_file = "boot.config");

        /**
         * @brief boot.config dosyasını diske yazar.
         */
        static bool save_boot_config(const boot_config& config, const std::string& config_file = "boot.config");

        /**
         * @brief Standalone oyun çalıştırma döngüsü.
         */
        static int run_standalone(int argc, char** argv);
    };

} // namespace runtime

