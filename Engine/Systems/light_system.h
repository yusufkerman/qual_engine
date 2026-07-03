#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../ECS/entity.h"
#include "../ECS/registry.h"
#include "../ECS/components.h"

struct light_data
{
    int       type;
    float     _pad0[3];    // 0-16

    glm::vec3 position;
    float     range;       // 16-32

    glm::vec3 direction;
    float     intensity;   // 32-48

    glm::vec3 color;
    float     inner_cone;  // 48-64

    float     outer_cone;
    float     _pad1[3];    // 64-80
};

static_assert(sizeof(light_data) == 80, "light_data size mismatch!");

class light_system
{
public:
    static std::vector<light_data> extract(const registry& reg);
};