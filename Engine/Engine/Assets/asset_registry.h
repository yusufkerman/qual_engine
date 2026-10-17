#pragma once

/**
 * @file asset_registry.h
 * @brief Varlıkların (asset) CPU belleğindeki tahsislerini ve nesilsel (generational) kayıtlarını tutan yönetici sınıf.
 */

#include "asset_structures.h"
#include <vector>
#include <map>
#include <string>
#include "asset_database.h"

namespace aria {

    /**
     * @class asset_registry
     * @brief Varlıkları türlerine göre yuvalarda (slots) saklar, önbellekler ve GUID üzerinden erişim sağlar.
     */
    class asset_registry {
    private:
        /**
         * @struct registry_slot
         * @brief Varlıkları nesilsel indeks (generational index) mantığıyla saklayan bellek yuvası.
         * @tparam T Saklanacak varlığın türü.
         */
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

        std::vector<registry_slot<skeleton_asset>> m_skeleton_slots;
        std::vector<uint32_t> m_skeleton_free_slots;
        std::map<asset_guid, handle<skeleton_asset>> m_skeleton_map;

        std::vector<registry_slot<animation_clip_asset>> m_animation_slots;
        std::vector<uint32_t> m_animation_free_slots;
        std::map<asset_guid, handle<animation_clip_asset>> m_animation_map;

        std::vector<registry_slot<skinned_model_asset>> m_skinned_model_slots;
        std::vector<uint32_t> m_skinned_model_free_slots;
        std::map<asset_guid, handle<skinned_model_asset>> m_skinned_model_map;

        /**
         * @brief Type trait: static_assert için derleme zamanında her zaman false döndüren yardımcı yapı.
         */
        template<typename> inline static constexpr bool always_false_v = false;

        /**
         * @brief İstenen varlık türüne ait GUID -> Handle haritasının referansını döndürür.
         * @tparam T Varlık türü (texture_asset, model_asset vb.)
         * @return const auto& İlgili std::map yapısına salt okunur referans.
         */
        template <typename T>
        [[nodiscard]] const auto& get_asset_map() const {
            if constexpr (std::is_same_v<T, texture_asset>)             return m_texture_map;
            else if constexpr (std::is_same_v<T, model_asset>)          return m_model_map;
            else if constexpr (std::is_same_v<T, material_asset>)       return m_material_map;
            else if constexpr (std::is_same_v<T, shader_asset>)         return m_shader_map;
            else if constexpr (std::is_same_v<T, skeleton_asset>)       return m_skeleton_map;
            else if constexpr (std::is_same_v<T, animation_clip_asset>) return m_animation_map;
            else if constexpr (std::is_same_v<T, skinned_model_asset>)  return m_skinned_model_map;
            else static_assert(always_false_v<T>, "Desteklenmeyen varlik turu (get_asset_map)!");
        }

    public:

        /**
         * @brief Kayıt defterini başlatır ve veritabanı bağlantısını kurar.
         * @param database Kullanılacak varlık veritabanı işaretçisi.
         */
        asset_registry(asset_database* database);

        /**
         * @brief Canlı bir handle referansını kullanarak, haritalardan tersten arama yapar ve varlığın GUID'sini bulur.
         * @note Sadece serileştirme (kaydetme) anında çağrılacağı için Linear Search (Doğrusal Arama) kullanır.
         *
         * @tparam T Varlık türü.
         * @param h Aranacak handle.
         * @return asset_guid Varlığın GUID değeri. Bulunamazsa boş/geçersiz (nil) GUID döner.
         */
        template <typename T>
        [[nodiscard]] asset_guid find_guid_by_handle(handle<T> h) const
        {
            if (!h.is_valid())
                return {};

            const auto& target_map = get_asset_map<T>();

            for (const auto& [guid, map_handle] : target_map)
            {
                if (map_handle.index == h.index && map_handle.generation == h.generation)
                {
                    return guid;
                }
            }

            return {};
        }

        /**
         * @brief Diskten gelen GUID'ye göre doğru yükleme fonksiyonunu otomatik olarak tetikler.
         * @note Model ve Material türleri için veritabanı üzerinden dosya yolunu (path) otomatik çözer.
         *
         * @tparam T Yüklenecek varlığın türü.
         * @param guid Varlığın benzersiz kimliği.
         * @return handle<T> Varlığa ait bellek tanıtıcısı.
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
                if (!m_database) return {};
                std::string path = m_database->resolve_path(guid);
                if (path.empty()) return {};

                return load_model(guid, path);
            }
            else if constexpr (std::is_same_v<T, material_asset>)
            {
                if (!m_database) return {};
                std::string path = m_database->resolve_path(guid);
                if (path.empty()) return {};

                return load_material(guid, path);
            }
            else if constexpr (std::is_same_v<T, skeleton_asset>)
            {
                if (!m_database) return {};
                std::string path = m_database->resolve_path(guid);
                if (path.empty()) return {};

                return load_skeleton(guid, path);
            }
            else if constexpr (std::is_same_v<T, animation_clip_asset>)
            {
                if (!m_database) return {};
                std::string path = m_database->resolve_path(guid);
                if (path.empty()) return {};

                return load_animation(guid, path);
            }
            else if constexpr (std::is_same_v<T, skinned_model_asset>)
            {
                if (!m_database) return {};
                std::string path = m_database->resolve_path(guid);
                if (path.empty()) return {};

                return load_skinned_model(guid, path);
            }
            else
            {
                static_assert(always_false_v<T>, "Desteklenmeyen varlik turu (load_asset_by_guid)!");
            }
        }

        // --- Doku (Texture) ---
        handle<texture_asset> load_texture(const asset_guid& guid);
        [[nodiscard]] texture_asset* get_texture(handle<texture_asset> h);
        void destroy_texture(handle<texture_asset> h);

        // --- Model (Mesh) ---
        handle<model_asset> load_model(const asset_guid& guid, const std::string& file_path);
        [[nodiscard]] model_asset* get_model(handle<model_asset> h);
        void destroy_model(handle<model_asset> h);

        /**
         * @brief Prosedürel olarak (kod ile) üretilmiş model verisini doğrudan kayıt defterine yazar.
         */
        handle<model_asset> add_procedural_model(const asset_guid& guid, const raw_model_data& raw_data);

        // --- İskelet (Skeleton) ---
        handle<skeleton_asset> load_skeleton(const asset_guid& guid, const std::string& file_path);
        handle<skeleton_asset> add_procedural_skeleton(const asset_guid& guid, const raw_skeleton_data& raw_data);
        [[nodiscard]] skeleton_asset* get_skeleton(handle<skeleton_asset> h);
        void destroy_skeleton(handle<skeleton_asset> h);

        // --- Animasyon Klibi (Animation Clip) ---
        handle<animation_clip_asset> load_animation(const asset_guid& guid, const std::string& file_path);
        handle<animation_clip_asset> add_procedural_animation(const asset_guid& guid, const raw_animation_data& raw_data);
        [[nodiscard]] animation_clip_asset* get_animation(handle<animation_clip_asset> h);
        void destroy_animation(handle<animation_clip_asset> h);

        // --- İskeletli Model (Skinned Model) ---
        handle<skinned_model_asset> load_skinned_model(const asset_guid& guid, const std::string& file_path);
        handle<skinned_model_asset> add_procedural_skinned_model(const asset_guid& guid, const raw_skinned_model_data& raw_data);
        [[nodiscard]] skinned_model_asset* get_skinned_model(handle<skinned_model_asset> h);
        void destroy_skinned_model(handle<skinned_model_asset> h);

        // --- Materyal (Material) ---
        handle<material_asset> load_material(const asset_guid& guid, const std::string& file_path);
        handle<material_asset> reload_material(const asset_guid& guid, const std::string& file_path);
        [[nodiscard]] material_asset* get_material(handle<material_asset> h);
        void destroy_material(handle<material_asset> h);

        // --- Shader ---
        handle<shader_asset> load_shader(const asset_guid& guid);
        [[nodiscard]] shader_asset* get_shader(handle<shader_asset> h);
        void destroy_shader(handle<shader_asset> h);

        /**
         * @brief Tüm varlık slotlarını, haritaları ve serbest yuva listelerini temizler.
         */
        void clear_registry();
    };

} // namespace aria