#pragma once

#include "asset_raw_structures.h"
#include "handle.h"
#include "../Externals/uuid/uuid.h" // Kütüphanenin bulunduğu klasör
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace aria {

    // Kütüphanenin tam namespace ismini vererek türü mühürledik
    using asset_guid = uuids::uuid;

    struct texture_asset {
        asset_guid guid;
        int width{ 0 };
        int height{ 0 };
        int channels{ 0 };
        raw_texture_data raw_data;
    };

    struct mesh_asset {
        unsigned int vao{ 0 };
        unsigned int vbo{ 0 };
        unsigned int ebo{ 0 };
        unsigned int index_count{ 0 };
        glm::vec3 local_center{ 0.0f };
        float local_radius{ 0.0f };
    };

    struct model_asset {
        asset_guid guid;
        std::vector<mesh_asset> sub_meshes;
        raw_model_data raw_data;
    };

    struct shader_asset {
        asset_guid guid;
        raw_shader_data raw_data;
    };

	// Material asset yapısı, bir malzemenin özelliklerini ve ilişkili kaynaklarını temsil eder
    struct material_asset {
        asset_guid guid;
        std::string name;

        render_mode mode{ render_mode::opaque };
        float alpha_threshold{ 0.1f };

        glm::vec3 base_color{ 1.0f };
        float specular_strength{ 0.5f };
        float shininess{ 32.0f };
        float normal_strength{ 1.0f };
        glm::vec2 texture_scale{ 1.0f };

        handle<texture_asset> diffuse_texture_handle;
        handle<texture_asset> normal_texture_handle;
        handle<texture_asset> specular_texture_handle;

        handle<shader_asset>  shader_handle;

    };

   

} // namespace aria