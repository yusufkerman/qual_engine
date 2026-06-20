#pragma once

#include <glm/glm.hpp>
#include <string>

struct material
{
    unsigned int shader_id = 0;

    glm::vec3 base_color = glm::vec3(1.0f);

    unsigned int diffuse_texture = 0;
    unsigned int normal_texture = 0;
    unsigned int specular_texture = 0;

    float normal_strength = 1.0f;
};