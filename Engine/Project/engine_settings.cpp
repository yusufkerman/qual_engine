/**
 * @file engine_settings.cpp
 * @brief Motor ayarları ve harici kod editörü entegrasyonu uygulaması.
 */

#include "engine_settings.h"
#include "../Externals/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <windows.h>
#include <shellapi.h>

using json = nlohmann::json;

namespace project {

    engine_settings& engine_settings::instance()
    {
        static engine_settings s_instance;
        return s_instance;
    }

    engine_settings::engine_settings()
    {
        detect_vscode();
        load();
    }

    std::filesystem::path engine_settings::get_settings_file_path() const
    {
        return std::filesystem::current_path() / "engine_settings.json";
    }

    std::string engine_settings::detect_vscode()
    {
        // 1. Standart Windows kullanıcı kurulum yolu (%LOCALAPPDATA%\Programs\Microsoft VS Code\Code.exe)
        char localAppData[MAX_PATH];
        if (GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH) > 0) {
            std::filesystem::path p1 = std::filesystem::path(localAppData) / "Programs" / "Microsoft VS Code" / "Code.exe";
            if (std::filesystem::exists(p1)) {
                m_detected_vscode_path = p1.string();
                return m_detected_vscode_path;
            }
        }

        // 2. Program Files yolu
        char progFiles[MAX_PATH];
        if (GetEnvironmentVariableA("ProgramFiles", progFiles, MAX_PATH) > 0) {
            std::filesystem::path p2 = std::filesystem::path(progFiles) / "Microsoft VS Code" / "Code.exe";
            if (std::filesystem::exists(p2)) {
                m_detected_vscode_path = p2.string();
                return m_detected_vscode_path;
            }
        }

        // 3. Program Files (x86) yolu
        char progFilesX86[MAX_PATH];
        if (GetEnvironmentVariableA("ProgramFiles(x86)", progFilesX86, MAX_PATH) > 0) {
            std::filesystem::path p3 = std::filesystem::path(progFilesX86) / "Microsoft VS Code" / "Code.exe";
            if (std::filesystem::exists(p3)) {
                m_detected_vscode_path = p3.string();
                return m_detected_vscode_path;
            }
        }

        // 4. Sistem PATH'inde 'code.cmd' veya 'code.exe' var mı?
        m_detected_vscode_path = "code";
        return m_detected_vscode_path;
    }

    std::string engine_settings::get_effective_editor_command()
    {
        switch (m_editor_type) {
            case code_editor_type::vs_code:
                return m_detected_vscode_path.empty() ? "code" : m_detected_vscode_path;
            case code_editor_type::visual_studio:
                return "devenv";
            case code_editor_type::custom:
                return m_custom_editor_path;
        }
        return "code";
    }

    bool engine_settings::open_in_editor(const std::string& filePath, const std::string& projectRoot)
    {
        // Electron tabanlı editörlerin (VS Code vb.) Node.js headless modunda çalışmasını engelle
        SetEnvironmentVariableA("ELECTRON_RUN_AS_NODE", NULL);
        std::string editor = get_effective_editor_command();
        std::string params = "";

        if (m_editor_type == code_editor_type::vs_code) {
            // VS Code için proje klasörünü workspace olarak aç ve dosyayı odağa al
            if (!projectRoot.empty()) {
                params += "\"" + projectRoot + "\" ";
            }
            if (!filePath.empty()) {
                params += "-g \"" + filePath + "\"";
            }
        }
        else {
            if (!filePath.empty()) {
                params = "\"" + filePath + "\"";
            }
            else if (!projectRoot.empty()) {
                params = "\"" + projectRoot + "\"";
            }
        }

        std::cout << "[EngineSettings] Editorde aciliyor: " << editor << " " << params << "\n";

        // Windows ShellExecute ile editörü arka planda çalıştır
        HINSTANCE result = ShellExecuteA(
            NULL,
            "open",
            editor.c_str(),
            params.c_str(),
            projectRoot.empty() ? NULL : projectRoot.c_str(),
            SW_SHOW
        );

        if (reinterpret_cast<INT_PTR>(result) <= 32) {
            // Eğer doğrudan çalıştırma başarısız olduysa, sistem shell'i üzerinden 'start' ile dene
            std::string cmdFallback = "/c start \"\" \"" + editor + "\" " + params;
            ShellExecuteA(NULL, "open", "cmd.exe", cmdFallback.c_str(), projectRoot.empty() ? NULL : projectRoot.c_str(), SW_HIDE);
        }

        return true;
    }

    void engine_settings::load()
    {
        std::filesystem::path settingsPath = get_settings_file_path();
        if (!std::filesystem::exists(settingsPath)) {
            return;
        }

        try {
            std::ifstream file(settingsPath);
            if (!file.is_open()) return;

            json j;
            file >> j;

            m_editor_type = static_cast<code_editor_type>(j.value("editor_type", 0));
            m_custom_editor_path = j.value("custom_editor_path", "");
            m_auto_reload_on_focus = j.value("auto_reload_on_focus", true);
        }
        catch (const std::exception& e) {
            std::cerr << "[EngineSettings] Ayar yukleme hatasi: " << e.what() << "\n";
        }
    }

    void engine_settings::save()
    {
        std::filesystem::path settingsPath = get_settings_file_path();
        try {
            json j;
            j["editor_type"] = static_cast<int>(m_editor_type);
            j["custom_editor_path"] = m_custom_editor_path;
            j["auto_reload_on_focus"] = m_auto_reload_on_focus;

            std::ofstream file(settingsPath);
            if (file.is_open()) {
                file << j.dump(4);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[EngineSettings] Ayar kaydetme hatasi: " << e.what() << "\n";
        }
    }

} // namespace project

