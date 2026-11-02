#pragma once

/**
 * @file project_exporter.h
 * @brief Açık olan kullanıcı projesini bağımsız, çalıştırılabilir oyun paketi (Standalone Game)
 * olarak derleyen ve dışa aktaran ihracat servisi.
 */

#include <string>
#include <vector>

namespace project {

    enum class export_pipeline_mode {
        use_project_settings = 0,
        psx_retro = 1,
        default_forward = 2
    };

    struct export_options {
        std::string output_directory;
        export_pipeline_mode pipeline_mode = export_pipeline_mode::psx_retro;
        bool compile_scripts = true;
        bool copy_engine_resources = true;
        bool copy_dependencies = true;
        int screen_width = 1280;
        int screen_height = 720;
        bool fullscreen = false;
        bool create_data_pak = true;
        bool delete_loose_assets_after_pack = false;
    };

    struct export_result {
        bool success = false;
        std::string error_message;
        std::vector<std::string> log_messages;
        std::string target_executable;
    };

    class project_exporter {
    public:
        /**
         * @brief Aktif projeyi belirtilen ayarlar doğrultusunda paketleyip dışa aktarır.
         * @param options Hedef dizin ve export seçenekleri.
         * @return export_result İşlem durumu, loglar ve hata mesajı.
         */
        static export_result export_project(const export_options& options);
    };

} // namespace project

