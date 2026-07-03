#pragma once
#include <string>
#include "handle.h"
#include "../Externals/uuid/uuid.h"

namespace aria {

    // Sahne kaydederken ve diskten okurken kullanılacak benzersiz kimlik
    using asset_guid = uuids::uuid;

    // Destekleyeceğimiz temel asset tipleri
    struct texture_asset {
        std::string path;
        int width{ 0 }, height{ 0 }, channels{ 0 };
        //  gl_id ASLA burada olmayacak! Sadece CPU ham verisi.
    };

    struct mesh_asset {
        std::string path;
        // Vertex ve Index dizileri (CPU verisi)
    };

    struct material_asset {
        std::string name;
        handle<texture_asset> diffuse_map;
        handle<texture_asset> normal_map;
    };

} // namespace aria