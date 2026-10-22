#pragma once

/**
 * @file asset_browser_view_model.h
 * @brief Varlık Tarayıcısı (Asset Browser) paneli için Model-View-ViewModel (MVVM) mimarisinin görünüm modeli katmanını tanımlar.
 */

#include <filesystem>
#include <functional>
#include <string>
#include "../Services/asset_entry.h"

namespace vesta {

    class asset_browser_system;
    class material_service;

    /**
     * @class asset_browser_view_model
     * @brief Arayüz (View) ile sistem (Model) arasında köprü kurar. Dosya işlemlerini koordine eder ve seçim durumunu yönetir.
     */
    class asset_browser_view_model {
    public:
        /**
         * @brief Varlık tarayıcısı görünüm modelini başlatır.
         *
         * @param browser_system Çekirdek dosya sistemi ve varlık işlemlerini yöneten servis.
         * @param mat_service Materyal üretim ve yönetim servisi.
         * @param on_asset_selected Bir varlık seçildiğinde (veya seçim temizlendiğinde) tetiklenecek geri çağrım (callback).
         * @param on_entity_deselect_requested Bir varlık seçildiğinde, sahnedeki Entity seçimini kaldırmak için tetiklenecek geri çağrım.
         */
        asset_browser_view_model(
            asset_browser_system& browser_system,
            material_service& mat_service,
            std::function<void(const std::string&)> on_asset_selected,
            std::function<void()> on_entity_deselect_requested
        );

        /**
         * @brief Dosya sisteminin kök (root) dizin düğümünü döndürür.
         */
        const asset_file_entry& root() const;

        const std::string& selected_path() const { return m_selected_path; }
        bool is_selected(const std::string& path) const { return m_selected_path == path; }

        /**
         * @brief Dosya sistemini yeniden tarar ve klasör hiyerarşisini günceller.
         */
        void refresh();

        /**
         * @brief Yalnızca iç durumu (state) güncelleyerek bir varlığı seçili hale getirir (Event fırlatmaz).
         */
        void select(const std::string& path);

        /**
         * @brief Bir varlığı seçili hale getirir ve bağlı editör sistemlerine (Inspector vb.) bildirim gönderir.
         */
        void select_and_notify(const std::string& path);

        /**
         * @brief Mevcut varlık seçimini temizler.
         */
        void clear_selection();

        // --- DOSYA VE VARLIK İŞLEMLERİ ---

        void request_create_material(const std::filesystem::path& target_dir, const std::string& name);
        void request_create_script(const std::filesystem::path& target_dir, const std::string& script_name);
        void request_create_folder(const std::filesystem::path& target_dir, const std::string& folder_name);
        void request_rename(const std::filesystem::path& target, const std::string& new_name);
        void request_duplicate(const std::filesystem::path& target);
        void request_delete(const std::filesystem::path& target);
        void request_move(const std::filesystem::path& src, const std::filesystem::path& dst_dir);

    private:
        asset_browser_system& m_browser_system;
        material_service& m_material_service;

        std::function<void(const std::string&)> m_on_asset_selected;
        std::function<void()> m_on_entity_deselect_requested;

        std::string m_selected_path;
    };

} // namespace vesta