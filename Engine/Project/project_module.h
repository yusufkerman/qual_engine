#pragma once

/**
 * @file project_module.h
 * @brief Açık olan kullanıcı projesinin GameModule.dll dosyasını çalışma zamanında
 * dinamik olarak bağlayan (LoadLibrary/FreeLibrary) ve script_factory ile senkronize eden servis.
 */

#include <string>
#include <vector>
#include <filesystem>

namespace project {

    /**
     * @class project_module
     * @brief Dinamik oyun modülü yaşam döngüsü yöneticisi.
     */
    class project_module {
    public:
        static project_module& instance();

        /**
         * @brief Belirtilen GameModule.dll dosyasını güvenli kopya üzerinden belleğe yükler ve scriptleri kaydeder.
         * @param dllPath Orijinal GameModule.dll dosya yolu.
         * @return true Yükleme başarılıysa.
         */
        bool load_module(const std::string& dllPath);

        /**
         * @brief Yüklü olan modülü bellekten boşaltır ve script kayıtlarını temizler.
         */
        void unload_module();

        /**
         * @brief Mevcut modülü yeniden yükler (Hot-Reload).
         */
        bool reload_module();

        /**
         * @brief Aktif olarak bir oyun modülü yüklü mü?
         */
        [[nodiscard]] bool is_loaded() const noexcept { return m_isLoaded; }

        /**
         * @brief Yüklü olan scriptlerin isim listesi.
         */
        [[nodiscard]] const std::vector<std::string>& get_loaded_scripts() const noexcept { return m_loadedScripts; }

        /**
         * @brief Yüklü modülün orijinal dosya yolu.
         */
        [[nodiscard]] const std::string& get_module_path() const noexcept { return m_originalDllPath; }

        /**
         * @brief Yüklü olan DLL'in statik motor bağlamını (gameplay::context) güncel motor durumu ile senkronize eder.
         */
        void sync_context() const;

        /**
         * @brief Proje scriptlerini arka planda derler (devenv.com veya MSBuild kullanarak).
         * @param projectRoot Proje ana dizini.
         * @param outLog Derleme konsol çıktısı.
         * @return true Derleme başarılıysa.
         */
        static bool compile_project_scripts(const std::string& projectRoot, std::string& outLog);

        /**
         * @brief GameModule.dll'in diskteki derleme tarihinin kaynak dosyalardan veya Engine.lib'den eski olup olmadığını denetler.
         */
        [[nodiscard]] static bool is_module_outdated(const std::string& projectRoot);

        /**
         * @brief Projenin Scripts dizinini, GameModule.vcxproj dosyasını ve script sınıflarını
         * otomatik tarayarak GameModule.cpp kayıt dosyasını hazırlar / günceller.
         * @param projectRoot Proje ana dizini.
         * @param outLog Bilgi veya hata mesajı.
         * @return true İskelet ve kayıtlar hazırsa.
         */
        static bool ensure_script_scaffolding(const std::string& projectRoot, std::string& outLog);

    private:
        project_module() = default;
        ~project_module();

        void* m_hModule = nullptr;
        void* m_syncContextFn = nullptr;
        bool m_isLoaded = false;
        std::string m_originalDllPath;
        std::string m_runtimeDllPath;
        std::vector<std::string> m_loadedScripts;
    };

} // namespace project


