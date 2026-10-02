#pragma once

/**
 * @file asset_traits.h
 * @brief (LEGACY) Erken aşama varlık (asset) veri yapıları taslağı.
 * @deprecated DİKKAT: Bu yapıların güncel ve tam halleri 'asset_structures.h' dosyasında bulunmaktadır.
 * Derleme (redefinition) hatalarını önlemek için bu dosyanın projeden kaldırılması tavsiye edilir.
 */

#include <string>
#include "handle.h"
#include "../Externals/uuid/uuid.h"

namespace aria {

    /**
     * @typedef asset_guid
     * @brief (Legacy) Sahne kaydederken ve diskten okurken kullanılacak benzersiz kimlik.
     */
    using asset_guid = uuids::uuid;

    /**
     * @struct texture_asset
     * @brief (Legacy) Doku varlığının erken aşama tanımı.
     * @note Sadece CPU verisini temsil eder, OpenGL kimlikleri (gl_id) barındırmaz.
     */
    struct texture_asset {
        std::string path;   ///< Dokunun disk üzerindeki yolu.
        int width{ 0 };     ///< Genişlik (piksel).
        int height{ 0 };    ///< Yükseklik (piksel).
        int channels{ 0 };  ///< Renk kanalı sayısı.
    };

    /**
     * @struct mesh_asset
     * @brief (Legacy) 3D ağ (mesh) varlığının erken aşama tanımı.
     */
    struct mesh_asset {
        std::string path;   ///< Mesh dosyasının disk üzerindeki yolu.

        // Not: Vertex ve Index dizileri (CPU verisi) bu taslakta henüz tanımlanmamış.
    };

    /**
     * @struct material_asset
     * @brief (Legacy) Materyal varlığının erken aşama tanımı.
     */
    struct material_asset {
        std::string name;                   ///< Materyalin adı.
        handle<texture_asset> diffuse_map;  ///< Temel renk (Diffuse/Albedo) doku tanıtıcısı.
        handle<texture_asset> normal_map;   ///< Normal haritası doku tanıtıcısı.
    };

} // namespace aria