#pragma once

#include <vector>
#include <string>
#include "vertex.h"

struct Mesh
{
    std::string name;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};