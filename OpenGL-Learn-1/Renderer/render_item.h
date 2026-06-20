#pragma once
#include <glm/glm.hpp>

struct render_item
{
    glm::mat4 model;

    unsigned int mesh_id;

    unsigned int shader_id;

    glm::vec3 base_color;

    unsigned int diffuse_texture;
};