#pragma once

#include <string>
#include <vector>
#include "../ECS/registry.h"
#include "../Systems/camera_system.h" // FULL INCLUDE

class scene
{
public:
    scene() = default;

    entity create_entity();
    void new_scene();

    registry& get_registry();
    const registry& get_registry() const;

    const std::vector<entity>& get_entities() const;
    void set_entities(const std::vector<entity>& entities);

    void set_active_camera(entity newCam);
    entity get_active_camera() const;

private:
    registry reg;
    std::vector<entity> entities;

    entity activeCamera = 0;
};