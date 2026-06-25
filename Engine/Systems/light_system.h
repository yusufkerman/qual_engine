#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../ECS/entity.h"
#include "../ECS/registry.h"
#include "../ECS/components.h"

struct light_data
{
    int type; // 0 directional, 1 point, 2 spot

    glm::vec3 position;
    glm::vec3 direction;

    glm::vec3 color;

    float intensity;

    float range;

    float inner_cone;
    float outer_cone;
};

class light_system
{
public:
    static std::vector<light_data> extract(const registry& reg);
};