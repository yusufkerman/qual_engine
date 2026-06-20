#pragma once
#include <vector>
#include "../Systems/light_system.h"
#include "../ECS/components.h"

struct light_batch
{
    std::vector<light_data> lights;

    void clear()
    {
        lights.clear();
    }
};