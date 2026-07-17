#include "asset_browser_system.h"
#include "material_service.h"
#include <algorithm>
#include <fstream>
#include <iostream>

#include "../Engine/asset_pipeline.h"

namespace vesta {

    asset_browser_system::asset_browser_system(
        const std::string& root_folder,
        aria::system* aria_sys,
        material_service* mat_service)
        : m_root_path(root_folder)
        , m_aria_system(aria_sys)
        , m_material_service(mat_service)
    {
        m_root_entry.name = root_folder;
        m_root_entry.full_path = std::filesystem::absolute(root_folder);
        m_root_entry.type = asset_type::folder;
    }

    void asset_browser_system::refresh() {
        m_root_entry.children.clear();
        if (std::filesystem::exists(m_root_path)) {
            scan_directory(m_root_path, m_root_entry);
        }
    }

    asset_type asset_browser_system::get_type_from_extension(const std::filesystem::path& path) {
        if (std::filesystem::is_directory(path)) return asset_type::folder;

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".dds") return asset_type::texture;
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".dae") return asset_type::model;
        if (ext == ".json" || ext == ".amat") return asset_type::material;

        return asset_type::unknown;
    }

    void asset_browser_system::scan_directory(const std::filesystem::path& dir_path, asset_file_entry& out_entry)
    {
        for (const auto& entry : std::filesystem::directory_iterator(dir_path))
        {
            if (entry.path().extension() == ".meta")
                continue;

            asset_file_entry file_item;
            file_item.full_path = entry.path();
            file_item.relative_path = std::filesystem::relative(entry.path(), m_root_path);
            file_item.name = entry.path().filename().string();
            file_item.type = get_type_from_extension(entry.path());

            file_item.guid = asset_pipeline::get_or_create_meta_guid(entry.path().string());

            if (file_item.type != asset_type::folder && m_aria_system && !file_item.guid.is_nil()) {
                m_aria_system->register_asset_path(file_item.guid, entry.path().string());
            }

            if (file_item.type == asset_type::folder)
                scan_directory(entry.path(), file_item);

            out_entry.children.push_back(file_item);
        }

        std::sort(out_entry.children.begin(), out_entry.children.end(),
            [](const asset_file_entry& a, const asset_file_entry& b)
            {
                if (a.type == asset_type::folder && b.type != asset_type::folder) return true;
                if (a.type != asset_type::folder && b.type == asset_type::folder) return false;
                return a.name < b.name;
            });
    }

    bool asset_browser_system::delete_asset(const std::filesystem::path& path) {
        try {
            bool ok = std::filesystem::remove_all(path) > 0;
            if (ok) {
                refresh();
            }
            return ok;
        }
        catch (...) {
            return false;
        }
    }

    bool asset_browser_system::rename_asset(const std::filesystem::path& path, const std::string& new_name) {
        try {
            std::filesystem::path new_path = path.parent_path() / (new_name + path.extension().string());
            if (!std::filesystem::exists(new_path)) {
                std::filesystem::rename(path, new_path);
                return true;
            }
        }
        catch (...) {}
        return false;
    }

    bool asset_browser_system::duplicate_asset(const std::filesystem::path& path) {
        asset_type type = get_type_from_extension(path);

        // 🚀 Materyal ise işi material_service'e devret
        if (type == asset_type::material && m_material_service) {
            aria::asset_guid source_guid = asset_pipeline::get_or_create_meta_guid(path.string());
            if (source_guid.is_nil()) return false;

            aria::asset_guid new_guid = m_material_service->duplicate_material(source_guid, path.parent_path());
            return !new_guid.is_nil();
        }

        // Materyal değilse eski genel dosya kopyalama mantığı aynen kalıyor
        try {
            std::filesystem::path ext = path.extension();
            std::filesystem::path dest = path.parent_path() / (path.stem().string() + "_Copy" + ext.string());

            int count = 1;
            while (std::filesystem::exists(dest)) {
                dest = path.parent_path() / (path.stem().string() + "_Copy_" + std::to_string(count++) + ext.string());
            }

            if (std::filesystem::is_directory(path)) std::filesystem::copy(path, dest, std::filesystem::copy_options::recursive);
            else std::filesystem::copy_file(path, dest);
            return true;
        }
        catch (...) { return false; }
    }

    bool asset_browser_system::create_material(
        const std::filesystem::path& folder_path,
        const std::string& name)
    {
        // 🚀 Materyal oluşturma artık tamamen material_service'in sorumluluğunda
        if (!m_material_service) {
            std::cerr << "[asset_browser_system] material_service bağlı değil, materyal oluşturulamadı.\n";
            return false;
        }

        aria::asset_guid new_guid = m_material_service->create_new_material(folder_path, name);
        return !new_guid.is_nil();
    }

} // namespace vesta