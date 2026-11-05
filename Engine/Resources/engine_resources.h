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
        inline const aria::asset_guid ps1_shader            = *uuids::uuid::from_string("b872a912-4c28-4f81-a67b-12a83ef549c2");
        inline const aria::asset_guid volumetric_shader           = *uuids::uuid::from_string("c5f9d91c-bb85-4af0-9ff9-4240a85df7fe");
        inline const aria::asset_guid volumetric_composite_shader = *uuids::uuid::from_string("d4e29cab-f4ef-4b1b-81f2-b54f7560cd16");
        inline const aria::asset_guid ssao_shader                 = *uuids::uuid::from_string("f34ffc95-47fd-4d89-b6b9-20016d2518e7");
        inline const aria::asset_guid ssao_blur_shader            = *uuids::uuid::from_string("79872aed-83d7-4cfa-bd48-00c3a65f6560");
        inline const aria::asset_guid ssao_composite_shader       = *uuids::uuid::from_string("63c18c7e-0ad4-46f5-a199-b258219e76e6");
        inline const aria::asset_guid gtao_shader                 = *uuids::uuid::from_string("8a43ccd3-2ab7-48ad-8aa1-f1e68613aec8");
        inline const aria::asset_guid ssgi_shader                 = *uuids::uuid::from_string("9b54dde4-3bc8-49be-9bb2-a2f79724bfd9");
        inline const aria::asset_guid post_process_shader         = *uuids::uuid::from_string("7c65eef5-4cd9-4acf-8cc3-b3a80835cae0");
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

