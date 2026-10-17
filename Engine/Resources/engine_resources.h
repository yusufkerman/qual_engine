#pragma once

/**
 * @file engine_resources.h
 * @brief Motor dahili kaynaklarının (built-in shaders, textures, meshes) çözümlenmesi,
 * sabit GUID eşleştirmeleri ve dosya yolu yönetimini sağlar.
 */

#include <string>
#include <filesystem>
#include <optional>
#include "../Externals/uuid/uuid.h"

namespace aria {
    using asset_guid = uuids::uuid;
    class system;
}

namespace engine_resources {

    namespace builtin_guid {
        // Sabit ve deterministik Motor Dahili Kaynak GUID'leri
        inline const aria::asset_guid default_shader        = *uuids::uuid::from_string("c10d2850-634a-4e41-bd53-add2e817e129");
        inline const aria::asset_guid skinned_default       = *uuids::uuid::from_string("cd5743f9-dbda-4c13-8a08-c41f6c9bfa71");
        inline const aria::asset_guid gizmo_shader          = *uuids::uuid::from_string("1a96340a-1192-4f27-b8fd-0439b686f63b");
        inline const aria::asset_guid grid_shader           = *uuids::uuid::from_string("ca6418ad-3833-40aa-9847-5bbc212a890a");
        inline const aria::asset_guid skybox_shader         = *uuids::uuid::from_string("ab217918-3c47-43c5-b01e-e4a5feae6a82");
        inline const aria::asset_guid shadow_shader         = *uuids::uuid::from_string("dab2ae4c-56df-4dcb-a781-82fabfacee3d");
        inline const aria::asset_guid shadow_skinned_shader = *uuids::uuid::from_string("e5c2194b-4b19-4824-a749-33b684cb32fa");
        inline const aria::asset_guid cube_mesh             = *uuids::uuid::from_string("d23a0aff-3fdc-5851-a6c3-e9623da3cd11");
        inline const aria::asset_guid sphere_mesh           = *uuids::uuid::from_string("f18b3e21-9541-4770-bc2d-5b3a8829f001");
    }

    /**
     * @brief Motor dahili kaynak kök dizinini döndürür (Engine/Resources veya standalone EngineResources/).
     */
    [[nodiscard]] const std::filesystem::path& get_resources_path();

    /**
     * @brief İsteğe bağlı olarak kaynak dizinini elle belirler.
     */
    void set_custom_resources_path(const std::filesystem::path& path);

    /**
     * @brief Verilen dosya yolunu veya sanal URI'yi çözer.
     * @param path Örn: "@engine/Shaders/default_shader", "Shaders/default_shader", "engine://builtin/cube"
     * @return Diskteki mutlak veya çözümlenmiş dosya yolu.
     */
    [[nodiscard]] std::string resolve_path(const std::string& path);

    /**
     * @brief Verilen yolun bir dahili motor kaynağı olup olmadığını belirler.
     */
    [[nodiscard]] bool is_engine_resource_path(const std::string& path);

    /**
     * @brief Verilen GUID'in motor dahili kaynağına ait olup olmadığını kontrol eder.
     */
    [[nodiscard]] bool is_builtin_guid(const aria::asset_guid& guid);

    /**
     * @brief Motor dahili kaynak yolu için sabit GUID değerini döndürür.
     */
    [[nodiscard]] std::optional<aria::asset_guid> get_builtin_guid_for_path(const std::string& path);

    /**
     * @brief Dahili GUID için sanal motor kaynak yolunu döndürür (@engine/...).
     */
    [[nodiscard]] std::string get_builtin_virtual_path_for_guid(const aria::asset_guid& guid);

    /**
     * @brief Tüm dahili motor kaynaklarını Aria varlık yöneticisine kaydeder.
     * Bu işlem projenin asset_manifest.json dosyasına bağımlı kalmaksızın motorun
     * her projede sorunsuz ayağa kalkmasını garanti eder.
     */
    void register_builtin_resources(aria::system* aria_sys);

} // namespace engine_resources

