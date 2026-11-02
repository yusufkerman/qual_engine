/**
 * @file asset_browser_system.cpp
 * @brief Varlık tarayıcısının disk okuma ve modifikasyon işlevlerinin mantıksal uygulaması.
 */

#include "asset_browser_system.h"
#include "../../../Core/path_utils.h"
#include "material_service.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
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

    void asset_browser_system::set_root_path(const std::filesystem::path& new_path) {
        m_root_path = new_path;
        m_root_entry.name = qual::path_to_utf8(new_path.filename());
        if (m_root_entry.name.empty()) {
            m_root_entry.name = qual::path_to_utf8(new_path);
        }
        m_root_entry.full_path = std::filesystem::absolute(new_path);
        m_root_entry.type = asset_type::folder;
        refresh();
    }

    asset_type asset_browser_system::get_type_from_extension(const std::filesystem::path& path) {
        if (std::filesystem::is_directory(path)) return asset_type::folder;

        std::string ext = qual::path_to_utf8(path.extension());
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".dds") return asset_type::texture;
        if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") return asset_type::model;
        if (ext == ".json" || ext == ".amat") return asset_type::material;

        return asset_type::unknown;
    }

    void asset_browser_system::scan_directory(const std::filesystem::path& dir_path, asset_file_entry& out_entry)
    {
        std::error_code ec;
        if (!std::filesystem::exists(dir_path, ec) || !std::filesystem::is_directory(dir_path, ec))
            return;

        for (const auto& entry : std::filesystem::directory_iterator(dir_path, ec))
        {
            // .meta dosyaları arayüzde gösterilmez, doğrudan ait oldukları dosya ile okunur.
            if (entry.path().extension() == ".meta")
                continue;

            asset_file_entry file_item;
            file_item.full_path = entry.path();
            file_item.relative_path = std::filesystem::relative(entry.path(), m_root_path, ec);
            file_item.name = qual::path_to_utf8(entry.path().filename());
            file_item.type = get_type_from_extension(entry.path());

            // Varlık (Asset) boru hattından GUID sorgulanır (Yoksa yeni .meta oluşturulur)
            const std::string entryPathU8 = qual::path_to_utf8(entry.path());
            file_item.guid = asset_pipeline::get_or_create_meta_guid(entryPathU8);

            // Klasör değilse ve geçerli bir GUID'si varsa motorun adres defterine (registry) kaydet
            if (file_item.type != asset_type::folder && m_aria_system && !file_item.guid.is_nil()) {
                m_aria_system->register_asset_path(file_item.guid, entryPathU8);
            }

            if (file_item.type == asset_type::folder) {
                scan_directory(entry.path(), file_item);
            }

            out_entry.children.push_back(file_item);
        }

        // Sıralama Mantığı: Klasörler her zaman üstte (A-Z), dosyalar altta (A-Z)
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
            const std::filesystem::path meta_path = qual::path_from_utf8(qual::path_to_utf8(path) + ".meta");
            bool ok = std::filesystem::remove_all(path) > 0;
            if (std::filesystem::exists(meta_path)) {
                std::error_code ec;
                std::filesystem::remove(meta_path, ec);
            }
            if (ok) {
                refresh();
            }
            return ok;
        }
        catch (const std::exception& e) {
            std::cerr << "[asset_browser_system] Dosya silme hatasi (" << qual::path_to_utf8(path) << "): " << e.what() << "\n";
            return false;
        }
    }

    bool asset_browser_system::rename_asset(const std::filesystem::path& path, const std::string& new_name) {
        if (new_name.empty()) {
            return false;
        }
        try {
            const bool is_dir = std::filesystem::is_directory(path);
            const std::string ext_u8 = is_dir ? std::string{} : qual::path_to_utf8(path.extension());
            std::string final_name_u8 = new_name;
            if (!is_dir && !ext_u8.empty() && !final_name_u8.ends_with(ext_u8)) {
                final_name_u8 += ext_u8;
            }

            std::filesystem::path new_path = path.parent_path() / qual::path_from_utf8(final_name_u8);
            if (new_path == path) {
                return true;
            }
            if (!std::filesystem::exists(new_path)) {
                std::filesystem::rename(path, new_path);

                const std::filesystem::path old_meta = qual::path_from_utf8(qual::path_to_utf8(path) + ".meta");
                const std::filesystem::path new_meta = qual::path_from_utf8(qual::path_to_utf8(new_path) + ".meta");
                if (std::filesystem::exists(old_meta)) {
                    std::error_code ec;
                    std::filesystem::rename(old_meta, new_meta, ec);
                }

                if (!is_dir && get_type_from_extension(new_path) == asset_type::material) {
                    const std::string new_stem_u8 = qual::path_to_utf8(new_path.stem());
                    const std::string new_path_u8 = qual::path_to_utf8(new_path);

                    try {
                        nlohmann::json mat_json;
                        {
                            std::ifstream in_file(new_path);
                            if (in_file.is_open()) {
                                in_file >> mat_json;
                            }
                        }
                        if (mat_json.is_object()) {
                            mat_json["name"] = new_stem_u8;
                            std::ofstream out_file(new_path);
                            if (out_file.is_open()) {
                                out_file << mat_json.dump(2);
                            }
                        }
                    }
                    catch (...) {
                    }

                    if (m_aria_system) {
                        aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(new_path_u8);
                        if (!guid.is_nil()) {
                            m_aria_system->register_asset_path(guid, new_path_u8);
                            auto handle = m_aria_system->registry()->load_material(guid, new_path_u8);
                            if (handle.is_valid()) {
                                if (auto* mat = m_aria_system->registry()->get_material(handle)) {
                                    mat->name = new_stem_u8;
                                }
                            }
                        }
                    }
                }

                refresh();
                return true;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[asset_browser_system] Dosya isimlendirme hatasi (" << qual::path_to_utf8(path) << "): " << e.what() << "\n";
        }
        return false;
    }

    bool asset_browser_system::duplicate_asset(const std::filesystem::path& path) {
        asset_type type = get_type_from_extension(path);

        // Materyal işlemleri, içindeki GUID referanslarının da kopyalanması gerektiği için özel servise yönlendirilir.
        if (type == asset_type::material && m_material_service) {
            aria::asset_guid source_guid = asset_pipeline::get_or_create_meta_guid(qual::path_to_utf8(path));
            if (source_guid.is_nil()) return false;

            aria::asset_guid new_guid = m_material_service->duplicate_material(source_guid, path.parent_path());
            return !new_guid.is_nil();
        }

        // Standart dosya (doku, model vb.) kopyalama mantığı
        try {
            const std::string ext_u8 = qual::path_to_utf8(path.extension());
            const std::string stem_u8 = qual::path_to_utf8(path.stem());
            std::filesystem::path dest = path.parent_path() / qual::path_from_utf8(stem_u8 + "_Copy" + ext_u8);

            int count = 1;
            while (std::filesystem::exists(dest)) {
                dest = path.parent_path() / qual::path_from_utf8(stem_u8 + "_Copy_" + std::to_string(count++) + ext_u8);
            }

            if (std::filesystem::is_directory(path)) {
                std::filesystem::copy(path, dest, std::filesystem::copy_options::recursive);
            }
            else {
                std::filesystem::copy_file(path, dest);
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[asset_browser_system] Dosya cogaltma hatasi (" << qual::path_to_utf8(path) << "): " << e.what() << "\n";
            return false;
        }
    }

    bool asset_browser_system::create_material(const std::filesystem::path& folder_path, const std::string& name)
    {
        if (!m_material_service) {
            std::cerr << "[asset_browser_system] material_service bagli degil, materyal olusturulamadi.\n";
            return false;
        }

        aria::asset_guid new_guid = m_material_service->create_new_material(folder_path, name);
        return !new_guid.is_nil();
    }

} // namespace vesta
