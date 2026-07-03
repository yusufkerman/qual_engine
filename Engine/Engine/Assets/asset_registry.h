#pragma once
#include "asset_structures.h"
#include <vector>
#include <map>

#include <string>

#include "asset_database.h"

namespace aria {

    class asset_registry {
    private:
        template<typename T>
        struct registry_slot {
            T asset;
            uint32_t generation{ 1 };
            bool is_allocated{ false };
        };

        asset_database* m_database = nullptr;

        std::vector<registry_slot<texture_asset>> m_texture_slots;
        std::vector<uint32_t> m_texture_free_slots;
        std::map<asset_guid, handle<texture_asset>> m_texture_map;

        std::vector<registry_slot<model_asset>> m_model_slots;
        std::vector<uint32_t> m_model_free_slots;
        std::map<asset_guid, handle<model_asset>> m_model_map;

        std::vector<registry_slot<material_asset>> m_material_slots;
        std::vector<uint32_t> m_material_free_slots;
        std::map<asset_guid, handle<material_asset>> m_material_map;

        std::vector<registry_slot<shader_asset>> m_shader_slots;
        std::vector<uint32_t> m_shader_free_slots;
        std::map<asset_guid, handle<shader_asset>> m_shader_map;

        template <typename T>
        [[nodiscard]] const auto& get_asset_map() const {
            if constexpr (std::is_same_v<T, texture_asset>)   return m_texture_map;
            if constexpr (std::is_same_v<T, model_asset>)     return m_model_map;
            if constexpr (std::is_same_v<T, material_asset>)  return m_material_map;
            if constexpr (std::is_same_v<T, shader_asset>)    return m_shader_map;
        }

    public:

        /**
         * @brief Canlı bir handle verip, haritalardan tersten arayarak GUID'sini (string) bulur.
         * @note Sadece kaydetme (Save) anında çağrılacağı için bellek şişirmeyen Linear Search kullanır.
         */
        template <typename T>
        [[nodiscard]] asset_guid find_guid_by_handle(handle<T> h) const
        {
            if (!h.is_valid())
                return {};

            const auto& target_map = get_asset_map<T>();

            for (const auto& [guid, map_handle] : target_map)
            {
                if (map_handle.index == h.index &&
                    map_handle.generation == h.generation)
                {
                    return guid;
                }
            }

            return {};
        }

        /**
         * @brief Diskten gelen GUID'ye göre doğru yükleme metodunu (`load_texture`, `load_model` vb.) tetikler.
         * @note Model ve Material için database üzerinden dosya yolunu (path) otomatik çözer.
         */
        template<typename T>
        handle<T> load_asset_by_guid(const asset_guid& guid)
        {
            if (guid.is_nil())
                return {};

            if constexpr (std::is_same_v<T, texture_asset>)
            {
                return load_texture(guid);
            }
            else if constexpr (std::is_same_v<T, shader_asset>)
            {
                return load_shader(guid);
            }
            else if constexpr (std::is_same_v<T, model_asset>)
            {
                if (!m_database)
                    return {};

                std::string path = m_database->resolve_path(guid);

                if (path.empty())
                    return {};

                return load_model(guid, path);
            }
            else if constexpr (std::is_same_v<T, material_asset>)
            {
                if (!m_database)
                    return {};

                std::string path = m_database->resolve_path(guid);

                if (path.empty())
                    return {};

                return load_material(guid, path);
            }

            return {};
        }



        asset_registry(asset_database* database);


        handle<texture_asset> load_texture(const asset_guid& guid);
        [[nodiscard]] texture_asset* get_texture(handle<texture_asset> h);
        void destroy_texture(handle<texture_asset> h);

        handle<model_asset> load_model(const asset_guid& guid, const std::string& file_path);
        [[nodiscard]] model_asset* get_model(handle<model_asset> h);
        void destroy_model(handle<model_asset> h);

        // 🚀 PROSEDÜREL (SANAL) YÜKLEME KAPISI: Hazır veriyi doğrudan slota yazar
        handle<model_asset> add_procedural_model(const asset_guid& guid, const raw_model_data& raw_data);

        handle<material_asset> load_material(const asset_guid& guid, const std::string& file_path);
        [[nodiscard]] material_asset* get_material(handle<material_asset> h);
        void destroy_material(handle<material_asset> h);

        handle<shader_asset> load_shader(const asset_guid& guid);
        [[nodiscard]] shader_asset* get_shader(handle<shader_asset> h);
        void destroy_shader(handle<shader_asset> h);

        void clear_registry();
    };

} // namespace aria