#pragma once
#include <string>
#include <vector>
#include "../ECS/registry.h"

class scene
{
public:
    scene() = default;
    scene(const scene&) = default;
    scene& operator=(const scene&) = default;

    entity create_entity();
    void new_scene();
    void save(const std::string& path);
    void load(const std::string& path);

    registry& get_registry();
    const registry& get_registry() const;
    const std::vector<entity>& get_entities() const;
    void set_entities(const std::vector<entity>& entities);

    void set_active_camera(entity e);
    entity get_active_camera() const;

private:
    registry reg;
    std::vector<entity> entities;

    entity activeCamera = 0;
};