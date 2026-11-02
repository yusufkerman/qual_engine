/**
 * @file gameplay_scene.cpp
 * @brief Gameplay sahne servisi uygulaması.
 */

#include "gameplay_scene.h"
#include "gameplay_context.h"
#include "../Scene/scene.h"
#include "../ECS/components.h"
#include "../Systems/physics_system.h"
#include "../Systems/physics_api.h"

namespace gameplay
{
    gameplay_entity scene_service::find_entity(const std::string& name)
    {
        auto* reg = context::get_registry();
        if (!reg)
        {
            return gameplay_entity();
        }

        for (const auto& [e, nameComp] : reg->get_all<name_component>())
        {
            if (nameComp.name == name)
            {
                return gameplay_entity(e, reg);
            }
        }

        return gameplay_entity();
    }

    gameplay_entity scene_service::get_entity(entity id)
    {
        auto* reg = context::get_registry();
        if (reg)
        {
            const auto& entities = reg->get_all_entities();
            if (entities.find(id) != entities.end())
            {
                return gameplay_entity(id, reg);
            }
        }
        return gameplay_entity();
    }

    gameplay_entity scene_service::create_entity(const std::string& name)
    {
        auto* reg = context::get_registry();
        if (!reg)
        {
            return gameplay_entity();
        }

        const entity e = reg->create_entity();
        reg->add_component<transform_component>(e, transform_component{});

        if (!name.empty())
        {
            reg->add_component<name_component>(e, name_component{ name });
        }

        return gameplay_entity(e, reg);
    }

    void scene_service::destroy(entity id)
    {
        auto* reg = context::get_registry();
        if (reg)
        {
            const auto& entities = reg->get_all_entities();
            if (entities.find(id) != entities.end())
            {
                reg->remove_entity(id);
            }
        }
    }

    void scene_service::destroy(gameplay_entity ent)
    {
        ent.destroy();
    }

    gameplay_entity scene_service::get_active_camera()
    {
        auto* s = context::get_active_scene();
        if (s)
        {
            const entity cam = s->get_active_camera();
            if (cam != 0)
            {
                return gameplay_entity(cam, &s->get_registry());
            }
        }
        return gameplay_entity();
    }

    raycast_hit scene_service::raycast(const glm::vec3& origin, const glm::vec3& direction, float max_distance, entity ignore_entity)
    {
        raycast_hit hit;
        auto* phys = context::get_physics_system();
        auto* reg = context::get_registry();
        if (!phys || !reg)
        {
            return hit;
        }

        auto res = phys->api().raycast(origin, direction, max_distance, ignore_entity);
        hit.has_hit = res.hit;
        hit.point = res.point;
        hit.normal = res.normal;
        hit.distance = res.distance;
        if (res.hit && res.hit_entity != 0)
        {
            hit.entity = gameplay_entity(res.hit_entity, reg);
        }
        return hit;
    }
}
