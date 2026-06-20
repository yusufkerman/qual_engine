#include "registry.h"

// =========================
// ENTITY
// =========================
entity registry::create_entity()
{
    entity e = next_entity++;
    entities.insert(e);
    return e;
}

void registry::add_entity(entity e)
{
    entities.insert(e);
}

void registry::remove_entity(entity e)
{
    entities.erase(e);

    transforms.erase(e);
    meshes.erase(e);
    cameras.erase(e);
    lights.erase(e);
    names.erase(e);
    bounds.erase(e);    
    materials.erase(e);
}

const std::unordered_set<entity>& registry::get_all_entities() const
{
    return entities;
}

// =========================
// CLONE SYSTEM
// =========================
void registry::clone_entity(entity e, registry& dst) const
{
    dst.add_entity(e);

    if (auto* c = get_component<transform_component>(e))
        dst.add_component<transform_component>(e, *c);

    if (auto* c = get_component<mesh_component>(e))
        dst.add_component<mesh_component>(e, *c);

    if (auto* c = get_component<camera_component>(e))
        dst.add_component<camera_component>(e, *c);

    if (auto* c = get_component<light_component>(e))
        dst.add_component<light_component>(e, *c);

    if (auto* c = get_component<name_component>(e))
        dst.add_component<name_component>(e, *c);
}