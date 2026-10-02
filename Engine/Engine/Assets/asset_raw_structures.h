#pragma once

/**
 * @file asset_raw_structures.h
 * @brief Diskten belleğe (CPU) yüklenen varlıkların ham (raw) veri yapılarını barındırır.
 * @note Bu yapılar, veriler GPU'ya aktarılana kadar geçici taşıyıcı görevi görürler.
 */

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "../Externals/uuid/uuid.h"

 // ---------------------------------------------------------
 // ÖN BİLDİRİMLER (FORWARD DECLARATIONS) VE TİP TANIMLAMALARI
 // ---------------------------------------------------------

namespace uuids {
    class uuid;
}

namespace aria {

    /**
     * @typedef asset_guid
     * @brief Motor genelinde varlıkları benzersiz olarak tanımlayan GUID türü (stduuid sarmalayıcısı).
     */
    using asset_guid = uuids::uuid;

    /**
     * @enum render_mode
     * @brief Materyalin çizim aşamasındaki saydamlık davranışını belirler.
     */
    enum class render_mode : int {
        opaque = 0, ///< Düz, saydamlık içermeyen nesneler (Örn: Duvar, taş).
        cutout = 1, ///< Keskin alfa testi (Alpha test) gerektiren nesneler (Örn: Yaprak, çit).
        transparent = 2  ///< Yarı saydam (Alpha blend) nesneler (Örn: Cam, su).
    };

    // ---------------------------------------------------------
    // HAM VARLIK YAPILARI (RAW DATA STRUCTURES)
    // ---------------------------------------------------------

    /**
     * @struct raw_texture_data
     * @brief Diskten okunan piksel verilerini CPU belleğinde tutar.
     */
    struct raw_texture_data {
        std::string debug_path;         ///< Varlığın yüklendiği dosya yolu (Hata ayıklama amaçlı).
        int width{ 0 };                 ///< Dokunun piksel cinsinden genişliği.
        int height{ 0 };                ///< Dokunun piksel cinsinden yüksekliği.
        int channels{ 0 };              ///< Renk kanalı sayısı (3: RGB, 4: RGBA vb.).
        std::vector<uint8_t> pixels;    ///< Ham piksel byte dizisi.

        /**
         * @brief GPU aktarımı tamamlandıktan sonra vektör kapasitesini sıfırlayarak RAM'i temizler.
         */
        void clear_raw_data() {
            pixels.clear();
            pixels.shrink_to_fit();
        }
    };

    /**
     * @struct raw_vertex
     * @brief Tekil bir köşeye (vertex) ait temel 3D model verilerini tutar.
     */
    struct raw_vertex {
        glm::vec3 position{ 0.0f };     ///< 3D uzaydaki konum.
        glm::vec3 normal{ 0.0f };       ///< Yüzey normali vektörü (Işıklandırma için).
        glm::vec2 tex_coord{ 0.0f };    ///< UV kaplama koordinatları.
        glm::vec3 tangent{ 0.0f };      ///< Tanjant vektörü (Normal mapping için).
    };

    /**
     * @struct raw_mesh_data
     * @brief Alt mesh (sub-mesh) verilerini ve yerel sınır (bounding) hesaplamalarını barındırır.
     */
    struct raw_mesh_data {
        std::vector<raw_vertex> vertices; ///< Köşe noktaları dizisi.
        std::vector<uint32_t> indices;    ///< Köşe indeksleri dizisi (Çizim sırası).

        glm::vec3 local_center{ 0.0f }; ///< Mesh'in yerel merkez noktası.
        float local_radius{ 0.0f };     ///< Mesh'in yerel sınırlayıcı küre yarıçapı (Frustum culling için).

        /**
         * @brief GPU aktarımı tamamlandıktan sonra vektörleri ve kapasitelerini sıfırlar.
         */
        void clear_raw_data() {
            vertices.clear();
            vertices.shrink_to_fit();
            indices.clear();
            indices.shrink_to_fit();
        }
    };

    /**
     * @struct raw_model_data
     * @brief Bir veya daha fazla alt mesh'ten oluşan ana 3D model yapısı.
     */
    struct raw_model_data {
        std::string debug_path;                     ///< Modelin yüklendiği dosya yolu.
        std::vector<raw_mesh_data> sub_meshes;      ///< Modele ait alt mesh (sub-mesh) koleksiyonu.

        /**
         * @brief Modele ait tüm alt meshlerin ham verilerini bellekten temizler.
         */
        void clear_raw_data() {
            for (auto& mesh : sub_meshes) {
                mesh.clear_raw_data();
            }
            sub_meshes.clear();
            sub_meshes.shrink_to_fit();
        }
    };

    /**
     * @struct raw_material_data
     * @brief Materyal dosyasından (JSON vb.) okunan görsel özellikleri ve bağlı dokuların GUID'lerini tutar.
     */
    struct raw_material_data {
        std::string debug_path;                     ///< Materyal dosyasının yolu.
        std::string name;                           ///< Materyal adı.

        render_mode mode{ render_mode::opaque };    ///< Çizim saydamlık modu.
        float alpha_threshold{ 0.1f };              ///< Cutout modu için alfa kesme eşiği.

        glm::vec3 base_color{ 1.0f };               ///< Temel renk çarpanı (Albedo).
        float specular_strength{ 0.5f };            ///< Parlaklık/yansıma şiddeti.
        float shininess{ 32.0f };                   ///< Parlaklık pürüzlülük/odak değeri.
        float normal_strength{ 1.0f };              ///< Normal haritası etki şiddeti.
        glm::vec2 texture_scale{ 1.0f };            ///< UV ölçeklendirme çarpanı (Tiling).

        aria::asset_guid shader_guid;               ///< Kullanılacak shader programının benzersiz kimliği.
        aria::asset_guid diffuse_texture_guid;      ///< Difüz (Albedo) dokusunun benzersiz kimliği.
        aria::asset_guid normal_texture_guid;       ///< Normal dokusunun benzersiz kimliği.
        aria::asset_guid specular_texture_guid;     ///< Speküler dokusunun benzersiz kimliği.

        /**
         * @brief Materyal verileri oldukça küçüktür (vektör içermez), bu nedenle boşaltma gerektirmez.
         */
        void clear_raw_data() {}
    };

    /**
     * @struct raw_shader_data
     * @brief Shader kod dosyalarından (.vert, .frag) okunan ham metin (string) verilerini barındırır.
     */
    struct raw_shader_data {
        std::string debug_path;         ///< Shader dosyalarının kök yolu.
        std::string vertex_source;      ///< Vertex shader kaynak kodu.
        std::string fragment_source;    ///< Fragment shader kaynak kodu.

        /**
         * @brief GPU derleme aşaması bittikten sonra metin belleklerini serbest bırakır.
         */
        void clear_raw_data() {
            vertex_source.clear();
            vertex_source.shrink_to_fit();
            fragment_source.clear();
            fragment_source.shrink_to_fit();
        }
    };

} // namespace aria