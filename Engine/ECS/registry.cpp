/**
 * @file registry.cpp
 * @brief EnTT Sparse-Set tabanlı modern ECS kayıt defteri implementasyonu.
 */

#include "registry.h"

namespace {

using all_engine_components = std::tuple<
    transform_component,
    mesh_component,
    camera_component,
    light_component,
    material_component,
    bounds_component,
    rigidbody_component,
    collider_component,
    native_script_component,
    skinned_mesh_component,
    skeleton_component,
    animator_component,
    name_component,
    hierarchy_component,
    bone_attachment_component,
    canvas_component,
    rect_transform_component,
    ui_text_component,
    ui_image_component,
    ui_button_component,
    audio_source_component,
    audio_listener_component
>;

template<typename Tuple, size_t... Is>
void copy_all_tuple_components(const registry& src_reg, entity src, entity dst, registry& dst_reg, std::index_sequence<Is...>)
{
    ([&]() {
        using Comp = std::tuple_element_t<Is, Tuple>;
        if (const auto* c = src_reg.get_component<Comp>(src))
        {
            dst_reg.add_component<Comp>(dst, *c);
        }
    }(), ...);
}

} // anonymous namespace

registry::registry()
{
    init();
}

registry::registry(const registry& other)
{
    init();
    for (entity e : other.m_entities)
    {
        other.clone_entity(e, *this);
    }
}

registry& registry::operator=(const registry& other)
{
    if (this != &other)
    {
        clear();
        for (entity e : other.m_entities)
        {
            other.clone_entity(e, *this);
        }
    }
    return *this;
}

registry::registry(registry&& other) noexcept
    : m_entt(std::move(other.m_entt)),
      m_entities(std::move(other.m_entities))
{
    rebind_pools();
}

registry& registry::operator=(registry&& other) noexcept
{
    if (this != &other)
    {
        m_entt = std::move(other.m_entt);
        m_entities = std::move(other.m_entities);
        rebind_pools();
    }
    return *this;
}

void registry::init()
{
    // ID 0, QualEngine genelinde geçersiz (null/root/invalid) varlığı temsil eder.
    // EnTT havuzunda indeks 0 rezerve edilir ve asla kullanıcıya dağıtılmaz.
    (void)m_entt.create(to_entt(0));
    rebind_pools();
}

void registry::rebind_pools()
{
    m_pool_transforms.set_registry(&m_entt);
    m_pool_meshes.set_registry(&m_entt);
    m_pool_cameras.set_registry(&m_entt);
    m_pool_lights.set_registry(&m_entt);
    m_pool_names.set_registry(&m_entt);
    m_pool_bounds.set_registry(&m_entt);
    m_pool_materials.set_registry(&m_entt);
    m_pool_rigidbodies.set_registry(&m_entt);
    m_pool_colliders.set_registry(&m_entt);
    m_pool_scriptables.set_registry(&m_entt);
    m_pool_hierarchies.set_registry(&m_entt);
    m_pool_skinned_meshes.set_registry(&m_entt);
    m_pool_skeletons.set_registry(&m_entt);
    m_pool_animators.set_registry(&m_entt);
    m_pool_bone_attachments.set_registry(&m_entt);
    m_pool_canvases.set_registry(&m_entt);
    m_pool_rect_transforms.set_registry(&m_entt);
    m_pool_ui_texts.set_registry(&m_entt);
    m_pool_ui_images.set_registry(&m_entt);
    m_pool_ui_buttons.set_registry(&m_entt);
    m_pool_audio_sources.set_registry(&m_entt);
    m_pool_audio_listeners.set_registry(&m_entt);
}

entity registry::create_entity()
{
    const auto ent = m_entt.create();
    const entity e = to_entity(ent);
    m_entities.insert(e);
    return e;
}

void registry::add_entity(entity e)
{
    if (e == 0) return;
    const auto ent = to_entt(e);
    if (!m_entt.valid(ent))
    {
        (void)m_entt.create(ent);
    }
    m_entities.insert(e);
}

void registry::remove_entity(entity e)
{
    if (e == 0) return;
    const auto ent = to_entt(e);
    if (m_entt.valid(ent))
    {
        m_entt.destroy(ent);
    }
    m_entities.erase(e);
}

const std::unordered_set<entity>& registry::get_all_entities() const noexcept
{
    return m_entities;
}

bool registry::valid(entity e) const noexcept
{
    return e != 0 && m_entt.valid(to_entt(e));
}

void registry::clear()
{
    m_entt.clear();
    m_entities.clear();
    (void)m_entt.create(to_entt(0));
}

void registry::copy_components_internal(entity srcEntity, entity dstEntity, registry& dstRegistry) const
{
    copy_all_tuple_components<all_engine_components>(*this, srcEntity, dstEntity, dstRegistry,
        std::make_index_sequence<std::tuple_size_v<all_engine_components>>{});
}

void registry::clone_entity(entity e, registry& dst) const
{
    dst.add_entity(e);
    copy_components_internal(e, e, dst);
}

entity registry::duplicate_entity(entity source)
{
    // 1. Yeni bir benzersiz varlık üret
    const entity newEntity = create_entity();

    // 2. Fonksiyonel bileşenleri yeni kimliğe yansıt
    copy_components_internal(source, newEntity, *this);

    // 3. İsim bileşenini çoğalt ve kopya olduğunu son ek ile belirt
    if (auto* nameComp = get_component<name_component>(newEntity))
    {
        nameComp->name += "_Copy";
    }
    else
    {
        add_component<name_component>(newEntity, name_component{ "Entity_Copy" });
    }

    // 4. Kopyalanan varlığın alt hiyerarşi ve fizik bağlantılarını sıfırla
    if (auto* hc = get_component<hierarchy_component>(newEntity))
    {
        hc->first_child = 0;
        hc->next_sibling = 0;
        hc->prev_sibling = 0;
    }

    if (auto* rb = get_component<rigidbody_component>(newEntity))
    {
        rb->body_id = JPH::BodyID();
    }

    if (auto* nsc = get_component<native_script_component>(newEntity))
    {
        nsc->instance = nullptr;
    }

    return newEntity;
}
