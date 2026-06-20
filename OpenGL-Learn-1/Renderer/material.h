#pragma once

#include <glm/glm.hpp>
#include <string>

struct material
{
    unsigned int shader_id = 0;

    glm::vec3 base_color = glm::vec3(1.0f);

    unsigned int diffuse_texture = 0;
};