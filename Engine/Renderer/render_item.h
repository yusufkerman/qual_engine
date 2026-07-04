#pragma once
#include <glm/glm.hpp>
#include "../Engine/Assets/aria.h"

struct render_item {
    glm::mat4 model{ 1.0f };

    // GPU'da o an aktif olan derlenmiş program kimliği (Batching için anahtar)
    aria::gpu_shader_id gpu_shader_id{ 0 };

    // Mesh bilgisi
    aria::handle<aria::model_asset> model_handle;
    size_t sub_mesh_index{ 0 };

    // Dokuların RAM biletleri
    aria::handle<aria::texture_asset> diffuse_handle;
    aria::handle<aria::texture_asset> normal_handle;
    aria::handle<aria::texture_asset> specular_handle;

    // Materyal parametreleri
    glm::vec3 base_color{ 1.0f };
    glm::vec2 texture_scale{ 1.0f };
    float normal_strength{ 1.0f };
    float specular_strength{ 0.5f };
    float shininess{ 32.0f };

    //Alpha ayarları
    aria::render_mode mode{ aria::render_mode::opaque };
    float alpha_threshold{ 0.1f };
};