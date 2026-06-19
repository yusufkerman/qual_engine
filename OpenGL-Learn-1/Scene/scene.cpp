#include "scene.h"

entity scene::create_entity()
{
    entity e = reg.create_entity();
    entities.push_back(e);
    return e;
}

void scene::new_scene()
{
    reg = registry();
    entities.clear();
    activeCamera = 0;
}

registry& scene::get_registry()
{
    return reg;
}

const registry& scene::get_registry() const
{
    return reg;
}

const std::vector<entity>& scene::get_entities() const
{
    return entities;
}

void scene::set_entities(const std::vector<entity>& newEntities)
{
    entities = newEntities;
}

void scene::set_active_camera(entity e)
{
    activeCamera = e;
}

entity scene::get_active_camera() const
{
    return activeCamera;
}