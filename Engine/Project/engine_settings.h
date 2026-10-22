#pragma once

/**
 * @file engine_settings.h
 * @brief Motor tercihleri, harici kod editörü ayarları ve kalıcı yapılandırma yöneticisi.
 */

#include <string>
#include <filesystem>

namespace project {

    enum class code_editor_type {
        vs_code = 0,
        visual_studio = 1,
        custom = 2
    };

    class engine_settings {
    public:
        static engine_settings& instance();

        void load();
        void save();

        [[nodiscard]] code_editor_type get_editor_type() const { return m_editor_type; }
        void set_editor_type(code_editor_type type) { m_editor_type = type; }

        [[nodiscard]] const std::string& get_custom_editor_path() const { return m_custom_editor_path; }
        void set_custom_editor_path(const std::string& path) { m_custom_editor_path = path; }

        [[nodiscard]] const std::string& get_detected_vscode_path() const { return m_detected_vscode_path; }

        [[nodiscard]] bool get_auto_reload_on_focus() const { return m_auto_reload_on_focus; }
        void set_auto_reload_on_focus(bool enable) { m_auto_reload_on_focus = enable; }

        /**
         * @brief Sistemdeki Visual Studio Code kurulumunu otomatik arar.
         */
        std::string detect_vscode();

        /**
         * @brief Aktif ayarlara göre kullanılacak nihai editör çalıştırılabilir dosya yolunu döner.
         */
        std::string get_effective_editor_command();

        /**
         * @brief Belirtilen dosya veya projeyi harici kod editöründe açar.
         * @param filePath Açılacak dosya yolu (boş ise sadece proje açılır).
         * @param projectRoot Proje ana dizini (Workspace klasörü).
         * @return Başarılı ise true.
         */
        bool open_in_editor(const std::string& filePath, const std::string& projectRoot = "");

    private:
        engine_settings();
        ~engine_settings() = default;

        code_editor_type m_editor_type{ code_editor_type::vs_code };
        std::string m_custom_editor_path;
        std::string m_detected_vscode_path;
        bool m_auto_reload_on_focus{ true };

        std::filesystem::path get_settings_file_path() const;
    };

} // namespace project

