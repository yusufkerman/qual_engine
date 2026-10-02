#pragma once

/**
 * @file asset_structures.h
 * @brief Kayıt defteri (registry) içerisinde tutulan ve motor tarafından doğrudan kullanılan ana varlık (asset) yapılarını tanımlar.
 */

#include "asset_raw_structures.h"
#include "handle.h"
#include "../Externals/uuid/uuid.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace aria {

    /**
     * @typedef asset_guid
     * @brief Varlıkları benzersiz şekilde tanımlamak için kullanılan GUID türü.
     */
    using asset_guid = uuids::uuid;

    /**
     * @struct texture_asset
     * @brief GPU'ya aktarılmaya hazır veya aktarılmış doku (texture) varlığı.
     */
    struct texture_asset {
        asset_guid guid;                ///< Dokunun benzersiz kimliği.
        int width{ 0 };                 ///< Genişlik (piksel).
        int height{ 0 };                ///< Yükseklik (piksel).
        int channels{ 0 };              ///< Renk kanalı sayısı.
        raw_texture_data raw_data;      ///< CPU üzerinde tutulan ham piksel verisi (GPU'ya yüklendikten sonra boşaltılır).
    };

    /**
     * @struct mesh_asset
     * @brief Tekil bir 3D ağın (mesh) GPU üzerindeki bellek tanımlayıcılarını ve yerel sınırlarını tutar.
     */
    struct mesh_asset {
        unsigned int vao{ 0 };          ///< Vertex Array Object (OpenGL Kimliği).
        unsigned int vbo{ 0 };          ///< Vertex Buffer Object (OpenGL Kimliği).
        unsigned int ebo{ 0 };          ///< Element Buffer Object (OpenGL Kimliği).
        unsigned int index_count{ 0 };  ///< Çizilecek toplam indeks sayısı.
        glm::vec3 local_center{ 0.0f }; ///< Sınırlayıcı küre için yerel merkez noktası.
        float local_radius{ 0.0f };     ///< Sınırlayıcı küre yarıçapı.
    };

    /**
     * @struct model_asset
     * @brief Bir veya daha fazla alt ağdan (sub-mesh) oluşan tam bir 3D model varlığı.
     */
    struct model_asset {
        asset_guid guid;                        ///< Modelin benzersiz kimliği.
        std::vector<mesh_asset> sub_meshes;     ///< Modele ait alt ağların (mesh) listesi.
        raw_model_data raw_data;                ///< CPU üzerinde tutulan ham model verisi.
    };

    /**
     * @struct shader_asset
     * @brief GPU üzerinde derlenecek olan gölgelendirici (shader) programı varlığı.
     */
    struct shader_asset {
        asset_guid guid;                ///< Shader'ın benzersiz kimliği.
        raw_shader_data raw_data;       ///< CPU üzerinde tutulan ham kaynak kod (derlendikten sonra serbest bırakılır).
    };

    /**
     * @struct material_asset
     * @brief 3D yüzeylerin görsel özelliklerini (renk, parlaklık) ve bağlı olduğu dokuların (texture) tanıtıcılarını tutar.
     */
    struct material_asset {
        asset_guid guid;                ///< Materyalin benzersiz kimliği.
        std::string name;               ///< Materyalin kullanıcı dostu adı.

        render_mode mode{ render_mode::opaque }; ///< Çizim saydamlık modu (Opaque, Cutout, Transparent).
        float alpha_threshold{ 0.1f };           ///< Cutout modu için alfa kesme eşik değeri.

        glm::vec3 base_color{ 1.0f };            ///< Temel renk çarpanı (Albedo).
        float specular_strength{ 0.5f };         ///< Yansıma şiddeti.
        float shininess{ 32.0f };                ///< Yüzey pürüzsüzlüğü/odak noktası.
        float normal_strength{ 1.0f };           ///< Normal haritası etki şiddeti.
        glm::vec2 texture_scale{ 1.0f };         ///< UV koordinat ölçeklendirmesi (Tiling).

        // --- BAĞLI VARLIK TANITICILARI (HANDLES) ---
        handle<texture_asset> diffuse_texture_handle;  ///< Temel renk (Albedo) dokusu tanıtıcısı.
        handle<texture_asset> normal_texture_handle;   ///< Normal haritası dokusu tanıtıcısı.
        handle<texture_asset> specular_texture_handle; ///< Yansıma (Specular) dokusu tanıtıcısı.

        handle<shader_asset>  shader_handle;           ///< Bu materyali çizerken kullanılacak shader programı tanıtıcısı.
    };

} // namespace aria