#pragma once

/**
 * @file aria.h
 * @brief Aria Varlık (Asset) ve Kaynak Yönetim Sisteminin ana arayüzünü tanımlar.
 */

#include "asset_registry.h"
#include "opengl_backend.h"
#include "gpu_resource_cache.h"
#include "../Externals/uuid/uuid.h"
#include <string>
#include "asset_database.h"

namespace aria {

    /// @brief Aria sistemi için rezerve edilmiş evrensel isim alanı UUID'si.
    inline constexpr uuids::uuid aria_namespace = uuids::uuid{ {
        0x6b, 0xa7, 0xb8, 0x10,
        0x9d, 0xad, 0x11, 0xd1,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    } };

    /**
     * @class system
     * @brief Varlık veritabanı, bellek kayıt defteri ve GPU önbelleğini koordine eden ana yönetim sınıfı.
     */
    class system {
    private:
        // KRİTİK NOT: Üyelerin (members) tanımlanma sırası C++'ta başlatılma sırasını belirler.
        // m_registry, m_database'e bağımlı olduğu için m_database önce tanımlanmalıdır!
        asset_database     m_database;
        asset_registry     m_registry;
        opengl_backend     m_backend;
        gpu_resource_cache m_cache;

    public:
        system();
        ~system();

        /**
         * @brief Sistemi başlatır, diskteki varlık manifestosunu yükler ve önbelleği hazırlar.
         */
        void init();

        /**
         * @brief Sistemi güvenle kapatır ve veritabanını diske kaydeder.
         */
        void shutdown();

        /**
         * @brief Bellekteki varlık adres defterini (manifest) fiziksel diske senkronize eder.
         */
        void save_asset_database();

        /**
         * @brief [SERA SAVER API] Canlı bir ARIA handle'ının arkasındaki kalıcı GUID verisini bulur.
         * @tparam T Varlık tipi (model_asset, texture_asset vb.)
         * @param h Varlığı işaret eden handle.
         * @return asset_guid Varlığa ait benzersiz kimlik (GUID).
         */
        template <typename T>
        [[nodiscard]] asset_guid get_guid_by_handle(handle<T> h) const
        {
            return m_registry.find_guid_by_handle<T>(h);
        }

        /**
         * @brief [SERA LOADER API] Diskten gelen GUID'ye ait varlığı otomatik yükler ve handle döndürür.
         * @tparam T Varlık tipi.
         * @param guid Yüklenecek varlığın benzersiz kimliği.
         * @return handle<T> Yüklenen varlığa ait handle.
         */
        template <typename T>
        handle<T> get_or_load_asset_by_guid(const asset_guid& guid)
        {
            return m_registry.load_asset_by_guid<T>(guid);
        }

        /**
         * @brief Bir varlık yolunu ve GUID'sini veritabanına kaydeder (Sadece bellekte).
         */
        void register_asset_path(const asset_guid& guid, const std::string& file_path);

        /**
         * @brief Verilen GUID'ye karşılık gelen fiziksel dosya yolunu döndürür.
         */
        [[nodiscard]] std::string resolve_asset_path(const asset_guid& guid) const;

        // =========================================================
        // DÜŞÜK SEVİYE API (Doğrudan Yüklemeler)
        // =========================================================

        handle<texture_asset>  import_texture(const asset_guid& guid, const std::string& file_path);
        handle<model_asset>    import_model(const asset_guid& guid, const std::string& file_path);
        handle<skinned_model_asset>   import_skinned_model(const asset_guid& guid, const std::string& file_path);
        handle<skeleton_asset>        import_skeleton(const asset_guid& guid, const std::string& file_path);
        handle<animation_clip_asset>  import_animation(const asset_guid& guid, const std::string& file_path);
        handle<material_asset> import_material(const asset_guid& guid, const std::string& file_path);
        handle<material_asset> reload_material(const asset_guid& guid, const std::string& file_path);
        handle<shader_asset>   import_shader(const asset_guid& guid, const std::string& file_path);
        handle<audio_clip_asset> import_audio(const asset_guid& guid, const std::string& file_path);

        // =========================================================
        // YÜKSEK SEVİYE API (Sanal Primitifler vb.)
        // =========================================================

        /**
         * @brief Motor içi sanal modelleri (küp, küre vb.) oluşturup içe aktarır.
         */
        handle<model_asset>    import_virtual_model(const std::string& file_path);

        // =========================================================
        // GPU KAYNAK ERİŞİM API (Render Aşaması İçin)
        // =========================================================

        [[nodiscard]] const material_asset* get_material_data(handle<material_asset> h);
        [[nodiscard]] gpu_texture_id        get_texture_id(handle<texture_asset> h);
        [[nodiscard]] submesh_runtime_info  get_submesh_info(handle<model_asset> h, size_t sub_mesh_index);
        [[nodiscard]] size_t                get_mesh_index_count(handle<model_asset> h, size_t sub_mesh_index);
        [[nodiscard]] submesh_runtime_info  get_skinned_submesh_info(handle<skinned_model_asset> h, size_t sub_mesh_index);
        [[nodiscard]] size_t                get_skinned_mesh_index_count(handle<skinned_model_asset> h, size_t sub_mesh_index);
        [[nodiscard]] const skeleton_asset* get_skeleton_data(handle<skeleton_asset> h);
        [[nodiscard]] const animation_clip_asset* get_animation_data(handle<animation_clip_asset> h);
        [[nodiscard]] const skinned_model_asset*  get_skinned_model_data(handle<skinned_model_asset> h);
        [[nodiscard]] gpu_shader_id         get_shader_id(handle<shader_asset> h);
        [[nodiscard]] const audio_clip_asset* get_audio_data(handle<audio_clip_asset> h);

        // =========================================================
        // BELLEK BOŞALTMA API
        // =========================================================

        void unload_texture(handle<texture_asset> h);
        void unload_model(handle<model_asset> h);
        void unload_skinned_model(handle<skinned_model_asset> h);
        void unload_skeleton(handle<skeleton_asset> h);
        void unload_animation(handle<animation_clip_asset> h);
        void unload_shader(handle<shader_asset> h);
        void unload_audio(handle<audio_clip_asset> h);

        /**
         * @brief GPU'daki ve ana bellekteki tüm kaynakları temizler, sistem sıfırlanır.
         */
        void clear_all_resources();

        /**
         * @brief Varlık kayıt defterine doğrudan erişim sağlar.
         */
        [[nodiscard]] asset_registry* registry() { return &m_registry; }
    };

} // namespace aria