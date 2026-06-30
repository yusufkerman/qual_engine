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

    if (e >= next_entity)
        next_entity = e + 1;
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

    rigidbodies.erase(e);
    colliders.erase(e);

	scriptable_components.erase(e);
}

const std::unordered_set<entity>& registry::get_all_entities() const
{
    return entities;
}

// =========================
// CLONE SYSTEM
// =========================
void registry::copy_components_internal(entity srcEntity, entity dstEntity, registry& dstRegistry) const
{
    if (auto* c = get_component<transform_component>(srcEntity))
        dstRegistry.add_component<transform_component>(dstEntity, *c);

    if (auto* c = get_component<mesh_component>(srcEntity))
        dstRegistry.add_component<mesh_component>(dstEntity, *c);

    if (auto* c = get_component<camera_component>(srcEntity))
        dstRegistry.add_component<camera_component>(dstEntity, *c);

    if (auto* c = get_component<light_component>(srcEntity))
        dstRegistry.add_component<light_component>(dstEntity, *c);

    if (auto* c = get_component<material_component>(srcEntity))
        dstRegistry.add_component<material_component>(dstEntity, *c);

    if (auto* c = get_component<bounds_component>(srcEntity))
        dstRegistry.add_component<bounds_component>(dstEntity, *c);

    if (auto* c = get_component<rigidbody_component>(srcEntity))
        dstRegistry.add_component<rigidbody_component>(dstEntity, *c);

    if (auto* c = get_component<collider_component>(srcEntity))
        dstRegistry.add_component<collider_component>(dstEntity, *c);

    if (auto* c = get_component<native_script_component>(srcEntity))
        dstRegistry.add_component<native_script_component>(dstEntity, *c);
}

// =================================================================
// CLONE SYSTEM
// =================================================================
void registry::clone_entity(entity e, registry& dst) const
{
    dst.add_entity(e);

    // Tüm standart komponentleri kopyala (ID değişmeden karşıya geçiyor)
    copy_components_internal(e, e, dst);

    // İsim bileşenini clone özelinde olduğu gibi aktar
    if (auto* c = get_component<name_component>(e))
        dst.add_component<name_component>(e, *c);
}

entity registry::duplicate_entity(entity source)
{
    // 1. Benzersiz yeni bir entity ID'si üret
    entity newEntity = create_entity();

    // 2. Ortak fonksiyon ile tüm fonksiyonel komponentleri yeni ID'ye kopyala
    copy_components_internal(source, newEntity, *this);

    // 3. İsim bileşenini kopyala ve duplicate kuralına göre sonuna "_Copy" ekle
    if (auto* c = get_component<name_component>(source))
    {
        auto& nameComp = add_component<name_component>(newEntity, *c);
        nameComp.name += "_Copy";
    }
    else
    {
        add_component<name_component>(newEntity, name_component{ "Entity_Copy" });
    }

    return newEntity;
}