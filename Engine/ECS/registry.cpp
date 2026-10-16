/**
 * @file registry.cpp
 * @brief ECS kayıt defterinin varlık döngülerini (Lifecycle) ve kopyalama işlemlerini uygulayan kodlar.
 */

#include "registry.h"

entity registry::create_entity()
{
    const entity e = next_entity++;
    entities.insert(e);
    return e;
}

void registry::add_entity(entity e)
{
    entities.insert(e);

    if (e >= next_entity)
    {
        next_entity = e + 1;
    }
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
    hierarchies.erase(e);
}

const std::unordered_set<entity>& registry::get_all_entities() const
{
    return entities;
}

void registry::copy_components_internal(entity srcEntity, entity dstEntity, registry& dstRegistry) const
{
    if (const auto* c = get_component<transform_component>(srcEntity))
        dstRegistry.add_component<transform_component>(dstEntity, *c);

    if (const auto* c = get_component<mesh_component>(srcEntity))
        dstRegistry.add_component<mesh_component>(dstEntity, *c);

    if (const auto* c = get_component<camera_component>(srcEntity))
        dstRegistry.add_component<camera_component>(dstEntity, *c);

    if (const auto* c = get_component<light_component>(srcEntity))
        dstRegistry.add_component<light_component>(dstEntity, *c);

    if (const auto* c = get_component<material_component>(srcEntity))
        dstRegistry.add_component<material_component>(dstEntity, *c);

    if (const auto* c = get_component<bounds_component>(srcEntity))
        dstRegistry.add_component<bounds_component>(dstEntity, *c);

    if (const auto* c = get_component<rigidbody_component>(srcEntity))
        dstRegistry.add_component<rigidbody_component>(dstEntity, *c);

    if (const auto* c = get_component<collider_component>(srcEntity))
        dstRegistry.add_component<collider_component>(dstEntity, *c);

    if (const auto* c = get_component<native_script_component>(srcEntity))
        dstRegistry.add_component<native_script_component>(dstEntity, *c);

    if (const auto* c = get_component<skinned_mesh_component>(srcEntity))
        dstRegistry.add_component<skinned_mesh_component>(dstEntity, *c);

    if (const auto* c = get_component<skeleton_component>(srcEntity))
        dstRegistry.add_component<skeleton_component>(dstEntity, *c);

    if (const auto* c = get_component<animator_component>(srcEntity))
        dstRegistry.add_component<animator_component>(dstEntity, *c);

    if (const auto* c = get_component<bone_attachment_component>(srcEntity))
        dstRegistry.add_component<bone_attachment_component>(dstEntity, *c);

    // Hiyerarşi gibi diğer kopyalanması gereken bileşenler varsa buraya eklenebilir.
}

void registry::clone_entity(entity e, registry& dst) const
{
    dst.add_entity(e);

    // Standart fonksiyonel bileşenlerin birebir aktarımı
    copy_components_internal(e, e, dst);

    // Kimlik/İsim bileşeninin birebir aktarımı (Kopya olduğu belli edilmez)
    if (const auto* c = get_component<name_component>(e))
    {
        dst.add_component<name_component>(e, *c);
    }
}

entity registry::duplicate_entity(entity source)
{
    // 1. Yeni bir varlık (Entity) kimliği üret
    const entity newEntity = create_entity();

    // 2. Fonksiyonel bileşenleri yeni kimliğe yansıt
    copy_components_internal(source, newEntity, *this);

    // 3. İsim bileşenini çoğalt ve kopya olduğunu son ek ile belirt
    if (const auto* c = get_component<name_component>(source))
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