// Dosya Yolu: Importers/mesh_generator.h
#pragma once

#include <vector>
#include <glm/glm.hpp>

struct generator_vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
};

struct generated_mesh_data {
    std::vector<generator_vertex> vertices;
    std::vector<unsigned int> indices;
};

namespace mesh_generator
{
    // 1x1x1 boyutlarında, merkezi (0,0,0) olan bir küp üretir
    generated_mesh_data generate_cube();

    // Verilen yarıçap ve detay kalitesine (slice/stack) göre küre üretir
    generated_mesh_data generate_sphere(float radius = 1.0f, unsigned int sector_count = 36, unsigned int stack_count = 18);
}