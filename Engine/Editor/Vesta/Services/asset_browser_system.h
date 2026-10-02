#pragma once

/**
 * @file asset_browser_system.h
 * @brief İşletim sistemi dosya dizinlerini tarayan ve Varlık (Asset) veritabanı ile senkronize eden temel servis sınıfı.
 */

#include "asset_entry.h"
#include <string>
#include <functional>
#include <filesystem>

namespace aria {
    class system;
}

namespace vesta {

    class material_service;

    /**
     * @class asset_browser_system
     * @brief Kök (root) klasörden itibaren dosyaları okur, türlerini belirler, .meta dosyalarını yönetir ve disk işlemlerini (silme, çoğaltma) gerçekleştirir.
     */
    class asset_browser_system {
    private:
        std::filesystem::path m_root_path;
        asset_file_entry m_root_entry;

        aria::system* m_aria_system{ nullptr };
        material_service* m_material_service{ nullptr };

        /**
         * @brief Dosya uzantısına bakarak varlığın (asset) türünü tespit eder.
         */
        asset_type get_type_from_extension(const std::filesystem::path& path);

        /**
         * @brief Verilen dizini özyinelemeli (recursive) olarak tarar ve klasör hiyerarşisini oluşturur.
         */
        void scan_directory(const std::filesystem::path& dir_path, asset_file_entry& out_entry);

    public:
        /**
         * @brief Varlık tarayıcı sistemini başlatır.
         *
         * @param root_folder Taranacak kök klasörün (varsayılan: "Assets") yolu.
         * @param aria_sys Varlıkları kaydedecek olan ana motor Aria sistemi.
         * @param mat_service Materyal üretim ve kopyalama işlemlerini devralacak servis.
         */
        asset_browser_system(
            const std::string& root_folder = "Assets",
            aria::system* aria_sys = nullptr,
            material_service* mat_service = nullptr
        );

        /**
         * @brief Kök dizinden itibaren tüm dosyaları yeniden tarayarak listeyi günceller.
         */
        void refresh();

        /**
         * @brief Güncel dosya hiyerarşisinin kök düğümünü döndürür.
         */
        const asset_file_entry& root_entry() const { return m_root_entry; }

        // ==========================================
        // DİSK VE DOSYA OPERASYONLARI
        // ==========================================

        bool delete_asset(const std::filesystem::path& path);
        bool rename_asset(const std::filesystem::path& path, const std::string& new_name);
        bool duplicate_asset(const std::filesystem::path& path);
        bool create_material(const std::filesystem::path& folder_path, const std::string& name);
    };

} // namespace vesta