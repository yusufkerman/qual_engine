#pragma once
#include <glm/glm.hpp>

struct render_item
{
    glm::mat4 model;

    unsigned int mesh_id;

    unsigned int shader_id;

    glm::vec3 base_color;

    unsigned int diffuse_texture;
    unsigned int normal_texture = 0;
    unsigned int specular_texture = 0;

    float normal_strength = 1.0f;

    glm::vec2 texture_scale{ 1.0f, 1.0f };
};