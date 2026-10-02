#pragma once

/**
 * @file gpu_resource_cache.h
 * @brief CPU belleğindeki varlıkların (asset) GPU'ya aktarılmasını (lazy loading) ve önbelleklenmesini yönetir.
 */

#include "asset_registry.h"
#include "render_backend.h"
#include <map>

namespace aria {

    /**
     * @struct submesh_runtime_info
     * @brief Bir alt ağın (sub-mesh) GPU üzerindeki kimliklerini ve görünürlük testleri (culling) için gerekli sınırlarını barındırır.
     */
    struct submesh_runtime_info {
        gpu_mesh_id gpu_id;             ///< GPU tarafındaki bellek nesneleri (VAO, VBO, EBO).
        glm::vec3 local_center{ 0.0f }; ///< Yerel merkez noktası (Sınırlayıcı küre için).
        float local_radius{ 0.0f };     ///< Sınırlayıcı küre yarıçapı.
    };

    /**
     * @class gpu_resource_cache
     * @brief Varlıkları yalnızca ihtiyaç anında (JIT) GPU'ya aktaran, eşleştiren ve bellek boşaltımını koordine eden sistem.
     */
    class gpu_resource_cache {
    private:
        asset_registry* m_registry{ nullptr };  ///< Kaynak CPU verilerini alabilmek için varlık kayıt defteri.
        render_backend* m_backend{ nullptr };   ///< Donanımla (OpenGL/Vulkan/DX) konuşacak olan soyut arayüz.

        std::map<handle<texture_asset>, gpu_texture_id> m_texture_cache;
        std::map<handle<model_asset>, std::vector<submesh_runtime_info>> m_model_cache;
        std::map<handle<shader_asset>, gpu_shader_id> m_shader_cache;

        gpu_texture_id m_fallback_texture_id{ 0 };  ///< Hatalı veya eksik dokular için yedek (fallback) doku kimliği.
        gpu_shader_id  m_fallback_shader_id{ 0 };   ///< Hatalı derlenen shader'lar için yedek (fallback) shader kimliği.

    public:
        /**
         * @brief GPU önbellek yöneticisini başlatır.
         * @param registry Varlık verilerine (CPU) erişim sağlayacak kayıt defteri.
         * @param backend Grafik API'si ile doğrudan iletişim kuracak olan arka uç (backend).
         */
        gpu_resource_cache(asset_registry* registry, render_backend* backend);
        ~gpu_resource_cache();

        /**
         * @brief Arka ucu (backend) başlatır ve fallback (yedek) kaynakları GPU'da oluşturur.
         */
        void init();

        /**
         * @brief Sistemi güvenli bir şekilde kapatır ve tüm GPU kaynaklarını serbest bırakır.
         */
        void shutdown();

        /**
         * @brief Varlığa ait dokunun GPU kimliğini döndürür. Eğer GPU'ya henüz yüklenmemişse yükler.
         */
        gpu_texture_id get_texture_id(handle<texture_asset> h);

        /**
         * @brief Varlığa ait alt mesh'in GPU kimliğini ve sınırlarını döndürür. GPU'ya yüklenmemişse yükler.
         */
        submesh_runtime_info get_submesh_info(handle<model_asset> h, size_t sub_mesh_index);

        /**
         * @brief Varlığa ait shader programının GPU kimliğini döndürür. Derlenmemişse derler ve yükler.
         */
        gpu_shader_id get_shader_id(handle<shader_asset> h);

        // --- BELLEK BOŞALTMA (EVICTION) İŞLEMLERİ ---

        void evict_texture(handle<texture_asset> h);
        void evict_model(handle<model_asset> h);
        void evict_shader(handle<shader_asset> h);

        /**
         * @brief Önbellekteki tüm kaynakları (yedek kaynaklar hariç) GPU belleklerinden siler ve listeleri temizler.
         */
        void clear_gpu_cache();
    };

} // namespace aria