#include "asset_browser_system.h"
#include <algorithm>
#include <fstream>
#include <iostream>

#include "../Engine/asset_pipeline.h"

namespace aria::editor {

    asset_browser_system::asset_browser_system(const std::string& root_folder, aria::system* aria_sys)
        : m_root_path(root_folder), m_aria_system(aria_sys)
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
        // Uzantıları küçük harfe çeviriyoruz
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".dds") return asset_type::texture;
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".dae") return asset_type::model;
        if (ext == ".json" || ext == ".amat") return asset_type::material; // Senin materyal uzantın

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

            // 🚀 BÜYÜLÜ DOKUNUŞ: Eğer geçerli bir dosya ise ve aria_system bağlıysa veritabanına kaydet
            if (file_item.type != asset_type::folder && m_aria_system && !file_item.guid.is_nil()) {
                m_aria_system->register_asset_path(file_item.guid, entry.path().string());
            }

            if (file_item.type == asset_type::folder)
                scan_directory(entry.path(), file_item);

            out_entry.children.push_back(file_item);
        }

        // Sıralama kodu aynen kalıyor...
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

            // 🔥 KRİTİK FIX
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
        try {
            std::filesystem::path ext = path.extension();
            std::filesystem::path dest = path.parent_path() / (path.stem().string() + "_Copy" + ext.string());

            // İsim çakışması önleme mantığı burada
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
        std::filesystem::path final_path = folder_path / (name + ".json");

        std::ofstream file(final_path);
        if (!file.is_open()) return false;

        // Varsayılan shader'ın guid'ini bul (default_shader.meta'dan)
        aria::asset_guid default_shader_guid = asset_pipeline::get_or_create_meta_guid("Shaders/default_shader");

        file << "{\n"
            << "  \"type\": \"material\",\n"
            << "  \"name\": \"" << name << "\",\n"
            << "  \"base_color\": [1.0, 1.0, 1.0, 1.0],\n"
            << "  \"specular_strength\": 0.5,\n"
            << "  \"shininess\": 32,\n"
            << "  \"normal_strength\": 1.0,\n"
            << "  \"shader_guid\": \"" << uuids::to_string(default_shader_guid) << "\",\n"
            << "  \"diffuse_texture_guid\": \"00000000-0000-0000-0000-000000000000\",\n"
            << "  \"normal_texture_guid\": \"00000000-0000-0000-0000-000000000000\",\n"
            << "  \"specular_texture_guid\": \"00000000-0000-0000-0000-000000000000\"\n"
            << "}";

        return true;
    }

} // namespace aria::editor