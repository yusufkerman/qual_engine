/**
 * @file engine_settings.cpp
 * @brief Motor ayarları ve harici kod editörü entegrasyonu uygulaması.
 */

#include "engine_settings.h"
#include "../Core/path_utils.h"
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
        wchar_t localAppData[MAX_PATH];
        if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH) > 0) {
            std::filesystem::path p1 = std::filesystem::path(localAppData) / L"Programs" / L"Microsoft VS Code" / L"Code.exe";
            if (std::filesystem::exists(p1)) {
                m_detected_vscode_path = qual::path_to_utf8(p1);
                return m_detected_vscode_path;
            }
        }

        // 2. Program Files yolu
        wchar_t progFiles[MAX_PATH];
        if (GetEnvironmentVariableW(L"ProgramFiles", progFiles, MAX_PATH) > 0) {
            std::filesystem::path p2 = std::filesystem::path(progFiles) / L"Microsoft VS Code" / L"Code.exe";
            if (std::filesystem::exists(p2)) {
                m_detected_vscode_path = qual::path_to_utf8(p2);
                return m_detected_vscode_path;
            }
        }

        // 3. Program Files (x86) yolu
        wchar_t progFilesX86[MAX_PATH];
        if (GetEnvironmentVariableW(L"ProgramFiles(x86)", progFilesX86, MAX_PATH) > 0) {
            std::filesystem::path p3 = std::filesystem::path(progFilesX86) / L"Microsoft VS Code" / L"Code.exe";
            if (std::filesystem::exists(p3)) {
                m_detected_vscode_path = qual::path_to_utf8(p3);
                return m_detected_vscode_path;
            }
        }

        // 4. Sistem PATH içinde Code.exe veya code.cmd ara
        wchar_t foundCodeExe[MAX_PATH] = { 0 };
        if (SearchPathW(NULL, L"Code.exe", NULL, MAX_PATH, foundCodeExe, NULL) > 0) {
            m_detected_vscode_path = qual::path_to_utf8(std::filesystem::path(foundCodeExe));
            return m_detected_vscode_path;
        }

        wchar_t foundCodeCmd[MAX_PATH] = { 0 };
        if (SearchPathW(NULL, L"code.cmd", NULL, MAX_PATH, foundCodeCmd, NULL) > 0) {
            m_detected_vscode_path = qual::path_to_utf8(std::filesystem::path(foundCodeCmd));
            return m_detected_vscode_path;
        }

        m_detected_vscode_path = "code";
        return m_detected_vscode_path;
    }

    std::string engine_settings::get_effective_editor_command()
    {
        switch (m_editor_type) {
            case code_editor_type::vs_code:
                if (m_detected_vscode_path.empty()) {
                    detect_vscode();
                }
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

        // Dosya ve proje yollarını Windows yerel formatına (ters bölü \) normalize et
        std::filesystem::path normFilePath;
        if (!filePath.empty()) {
            normFilePath = qual::path_from_utf8(filePath).make_preferred();
        }
        std::filesystem::path normProjectRoot;
        if (!projectRoot.empty()) {
            normProjectRoot = qual::path_from_utf8(projectRoot).make_preferred();
        }

        std::wstring wParams = L"";
        if (m_editor_type == code_editor_type::vs_code) {
            // VS Code için:
            // 1. -r (reuse-window): Açık olan pencereye bağlan
            // 2. Proje kök klasörünü workspace olarak aç
            // 3. Dosyayı -g parametresi ile 1. satırda aç
            wParams = L"-r ";
            if (!normProjectRoot.empty()) {
                wParams += L"\"" + normProjectRoot.wstring() + L"\" ";
            }
            if (!normFilePath.empty()) {
                wParams += L"-g \"" + normFilePath.wstring() + L":1\"";
            }
        }
        else {
            if (!normFilePath.empty()) {
                wParams = L"\"" + normFilePath.wstring() + L"\"";
            }
            else if (!normProjectRoot.empty()) {
                wParams = L"\"" + normProjectRoot.wstring() + L"\"";
            }
        }

        std::wcout << L"[EngineSettings] Editorde aciliyor: " << qual::path_from_utf8(editor).wstring() << L" " << wParams << L"\n";

        std::wstring wEditor = qual::path_from_utf8(editor).wstring();
        std::wstring wDir = normProjectRoot.empty() ? L"" : normProjectRoot.wstring();

        // Windows ShellExecuteW ile editörü arka planda Unicode güvenli çalıştır
        HINSTANCE result = ShellExecuteW(
            NULL,
            L"open",
            wEditor.c_str(),
            wParams.c_str(),
            wDir.empty() ? NULL : wDir.c_str(),
            SW_SHOW
        );

        if (reinterpret_cast<INT_PTR>(result) <= 32) {
            std::wstring cmdFallback = L"/c start \"\" \"" + wEditor + L"\" " + wParams;
            ShellExecuteW(NULL, L"open", L"cmd.exe", cmdFallback.c_str(), wDir.empty() ? NULL : wDir.c_str(), SW_HIDE);
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
            m_last_project_path = j.value("last_project_path", "");

            // Geriye dönük mojibake otomatik onarımı (YazÄ±lÄ±m -> Yazılım)
            const std::string badSub = "Yaz\xC3\x84\xC2\xB1l\xC3\x84\xC2\xB1m";
            const std::string badSub2 = "Yaz\xC4\x84\xC2\xB1l\xC4\x84\xC2\xB1m";
            const std::string badSub3 = "Yaz\u00c4\u00b1l\u00c4\u00b1m";
            const std::string goodSub = "\x59\x61\x7A\xC4\xB1\x6C\xC4\xB1\x6D";
            auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
                if (from.empty()) return;
                size_t start_pos = 0;
                while((start_pos = str.find(from, start_pos)) != std::string::npos) {
                    str.replace(start_pos, from.length(), to);
                    start_pos += to.length();
                }
            };
            replaceAll(m_last_project_path, badSub, goodSub);
            replaceAll(m_last_project_path, "YazÄ±lÄ±m", goodSub);
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
            j["last_project_path"] = m_last_project_path;

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

