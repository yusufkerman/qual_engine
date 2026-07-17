#pragma once
#include "../Adapters/IEditableWorld.h"
#include "../CoreState/command_stack.h"
#include "../../../Engine/Assets/aria.h"
#include <string>
#include <filesystem>

namespace vesta {

    class material_service {
    private:
        IEditableWorld& m_world;
        command_stack& m_command_stack;

    public:
        material_service(IEditableWorld& world, command_stack& cmd_stack);
        ~material_service() = default;

        // Yeni bir malzeme dosyası (.amat veya .json) oluşturur ve ARIA'ya kaydeder
        aria::asset_guid create_new_material(const std::filesystem::path& directory_path, const std::string& name = "new_material");

        // Mevcut bir malzemeyi çoğaltır (Duplicate)
        aria::asset_guid duplicate_material(const aria::asset_guid& source_guid, const std::filesystem::path& target_directory);

        // Bir malzemenin içeriğini (örneğin shader veya albedo rengini) kaydeder
        bool save_material_changes(const aria::asset_guid& guid, const std::string& serialized_data);
    };

} // namespace vesta