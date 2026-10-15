/**
 * @file gameplay_scene.cpp
 * @brief Gameplay sahne servisi uygulaması.
 */

#include "gameplay_scene.h"
#include "gameplay_context.h"
#include "../Scene/scene.h"
#include "../ECS/components.h"

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
}
