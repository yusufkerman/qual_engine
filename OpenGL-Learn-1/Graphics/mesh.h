#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "vertex.h"

struct Mesh
{
    std::string name;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    glm::vec3 localCenter{ 0.0f };
    float     localRadius = 0.0f;
};