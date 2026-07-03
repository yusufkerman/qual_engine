#pragma once
#include "asset_registry.h"
#include "opengl_backend.h"
#include "gpu_resource_cache.h"
#include "../Externals/uuid/uuid.h"
#include <string>
#include "asset_database.h"

namespace aria {

    inline constexpr uuids::uuid aria_namespace = uuids::uuid{ {
        0x6b, 0xa7, 0xb8, 0x10,
        0x9d, 0xad, 0x11, 0xd1,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    } };

    class system {
    private:
        asset_registry     m_registry;
        opengl_backend     m_backend;
        gpu_resource_cache m_cache;
        asset_database     m_database;

    public:
        system();
        ~system();

        void init();
        void shutdown();

        void save_asset_database();

        /**
        * @brief [SERA SAVER API] Canlı bir ARIA handle'ının arkasındaki kalıcı GUID verisini bulur.
        * @note Sorumluluğu doğrudan alt katmandaki asset_registry'ye paslar.
        */
        template <typename T>
        [[nodiscard]] asset_guid get_guid_by_handle(handle<T> h) const
        {
            return m_registry.find_guid_by_handle<T>(h);
        }

        /**
         * @brief [SERA LOADER API] Diskten gelen GUID'ye ait asset'i otomatik yükler ve handle döner.
         * @note Tip kontrollerini ve database'den path çözme işini alt katmanda halleder.
         */
        template <typename T>
        handle<T> get_or_load_asset_by_guid(const asset_guid& guid)
        {
            return m_registry.load_asset_by_guid<T>(guid);
        }


        void register_asset_path(const asset_guid& guid, const std::string& file_path);
        [[nodiscard]] std::string resolve_asset_path(const asset_guid& guid) const;

        // --- Low-Level API (Tam kontrollü yüklemeler için) ---
        handle<texture_asset>  import_texture(const asset_guid& guid, const std::string& file_path);
        handle<model_asset>    import_virtual_model(const asset_guid& guid, const std::string& file_path);
        handle<material_asset> import_material(const asset_guid& guid, const std::string& file_path);
        handle<shader_asset>   import_shader(const asset_guid& guid, const std::string& file_path);

        // --- High-Level API (Primitifler ve otomatik path eşlemeleri için) ---
        handle<model_asset>    import_virtual_model(const std::string& file_path);

        // 🎨 GPU Kaynak API (MeshRenderer'ın Draw anında sorguladığı yer)
        [[nodiscard]] const material_asset* get_material_data(handle<material_asset> h);
        [[nodiscard]] gpu_texture_id        get_texture_id(handle<texture_asset> h);
        [[nodiscard]] gpu_mesh_id           get_mesh_gpu_id(handle<model_asset> h, size_t sub_mesh_index);
        [[nodiscard]] size_t                get_mesh_index_count(handle<model_asset> h, size_t sub_mesh_index);
        [[nodiscard]] gpu_shader_id         get_shader_id(handle<shader_asset> h);

        // 🗑️ Boşaltma API
        void unload_texture(handle<texture_asset> h);
        void unload_model(handle<model_asset> h);
        void unload_shader(handle<shader_asset> h);

        void clear_all_resources();

        [[nodiscard]] asset_registry* registry() { return &m_registry; }
    };

} // namespace aria