#pragma once
#include "asset_entry.h"
#include <string>
#include <functional>
#include <filesystem>

// Forward declaration: aria::system'ı dahil etmeden pointer olarak kullanabilmek için
namespace aria {
    class system;
}

namespace aria::editor {

    class asset_browser_system {
    private:
        std::filesystem::path m_root_path;
        asset_file_entry m_root_entry;
        aria::system* m_aria_system{ nullptr }; // 🚀 Aria sistemine gizli referans linki

        asset_type get_type_from_extension(const std::filesystem::path& path);
        void scan_directory(const std::filesystem::path& dir_path, asset_file_entry& out_entry);

    public:
        // 🚀 Kurucu fonksiyona opsiyonel aria::system* eklendi
        asset_browser_system(const std::string& root_folder = "Assets", aria::system* aria_sys = nullptr);

        void refresh();
        const asset_file_entry& root_entry() const { return m_root_entry; }

        // --- DİSK OPERASYONLARI API ---
        bool delete_asset(const std::filesystem::path& path);
        bool rename_asset(const std::filesystem::path& path, const std::string& new_name);
        bool duplicate_asset(const std::filesystem::path& path);
        bool create_material(const std::filesystem::path& folder_path, const std::string& name);
    };

}