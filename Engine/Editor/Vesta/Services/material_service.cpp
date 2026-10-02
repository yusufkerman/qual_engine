/**
 * @file material_service.cpp
 * @brief Materyal servisinin mantıksal uygulaması. Dosya IO ve GUID senkronizasyon işlemlerini içerir.
 */

#include "material_service.h"
#include "../../../Engine/asset_pipeline.h" 
#include "../../../Externals/uuid/uuid.h"   
#include <fstream>
#include <iostream>

namespace vesta {

    material_service::material_service(IEditableWorld& world, command_stack& cmd_stack)
        : m_world(world), m_command_stack(cmd_stack) {
    }

    aria::asset_guid material_service::create_new_material(const std::filesystem::path& directory_path, const std::string& name) {
        // Motorun standardı olan .json uzantısı ile dosya yolu oluşturulur
        std::filesystem::path full_path = directory_path / (name + ".json");

        std::ofstream file(full_path);
        if (!file.is_open()) {
            std::cerr << "[Vesta MaterialService] Hata: Materyal dosyasi olusturulamadi (" << full_path << ")\n";
            return {};
        }

        // Varsayılan shader için sistemden GUID sorgulanır
        aria::asset_guid default_shader_guid = asset_pipeline::get_or_create_meta_guid("Shaders/default_shader");

        // Standart materyal JSON şablonu dosyaya yazılır
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
        file.close();

        // Dünya adaptörü (IEditableWorld) üzerinden bu yeni dosyaya ait GUID sorgulanır ve döndürülür
        aria::asset_guid new_guid = m_world.get_guid_from_path(full_path.string());

        std::cout << "[Vesta MaterialService] Yeni materyal olusturuldu: " << full_path.string() << "\n";

        return new_guid;
    }

    aria::asset_guid material_service::duplicate_material(const aria::asset_guid& source_guid, const std::filesystem::path& target_directory) {

        // IEditableWorld üzerinden kaynak GUID'in gerçek disk yolu çözümlenir
        std::string source_path_str = m_world.resolve_asset_path(source_guid);
        if (source_path_str.empty()) {
            return {};
        }

        std::filesystem::path source_path(source_path_str);

        // İsim çakışmasını önlemek için "_Copy" takısı eklenir
        std::filesystem::path target_path = target_directory / (source_path.stem().string() + "_Copy" + source_path.extension().string());

        int count = 1;
        while (std::filesystem::exists(target_path)) {
            target_path = target_directory / (source_path.stem().string() + "_Copy_" + std::to_string(count++) + source_path.extension().string());
        }

        try {
            std::filesystem::copy_file(source_path, target_path, std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[Vesta MaterialService] Materyal kopyalama hatasi: " << e.what() << "\n";
            return {};
        }

        // Yeni kopyalanan dosyayı sisteme tanıtıp motorun atadığı GUID döndürülür
        return m_world.get_guid_from_path(target_path.string());
    }

    bool material_service::save_material_changes(const aria::asset_guid& guid, const std::string& serialized_data) {
        if (guid.is_nil()) {
            return false;
        }

        std::string path_str = m_world.resolve_asset_path(guid);
        if (path_str.empty()) {
            return false;
        }

        std::ofstream file(path_str);
        if (!file.is_open()) {
            std::cerr << "[Vesta MaterialService] Hata: Materyal dosyasi kayit icin acilamadi (" << path_str << ")\n";
            return false;
        }

        file << serialized_data;
        file.close();

        // Sahnenin (dünyanın) değiştirildiğini sisteme bildir (Kaydedilmemiş değişiklikler için)
        m_world.mark_scene_dirty();
        return true;
    }

} // namespace vesta