#include "camera_system.h"
#include "../Scene/scene.h"
#include "../ECS/components.h"

#include <iostream>

void camera_system::update(scene& scn)
{
    auto& reg = scn.get_registry();

    entity bestCamera = 0;
    bool foundPrimary = false;

    // 1. PRIMARY FIX + FIND
    for (auto e : scn.get_entities())
    {
        if (!reg.has_component<camera_component>(e))
            continue;

        auto* cam = reg.get_component<camera_component>(e);

        if (cam->primary)
        {
            if (!foundPrimary)
            {
                foundPrimary = true;
                bestCamera = e;
            }
            else
            {
                cam->primary = false;
            }
        }
    }

    // 2. fallback
    if (!foundPrimary)
    {
        for (auto e : scn.get_entities())
        {
            if (!reg.has_component<camera_component>(e))
                continue;

            reg.get_component<camera_component>(e)->primary = true;
            bestCamera = e;
            break;
        }
    }

    // 3. change detection (LOG HERE)
    entity prev = m_lastCamera;

    if (bestCamera != 0)
    {
        scn.set_active_camera(bestCamera);
    }

    if (bestCamera != prev)
    {
        std::cout << "[CameraSystem] Active camera changed: "
            << prev << " -> " << bestCamera << "\n";
        m_lastCamera = bestCamera;
    }
}