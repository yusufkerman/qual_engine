/**
 * @file gameplay_entity.cpp
 * @brief Gameplay entity sarmalayıcı uygulaması.
 */

#include "gameplay_entity.h"
#include "gameplay_context.h"
#include "../ECS/components.h"

namespace gameplay
{
    gameplay_entity::gameplay_entity(entity id, registry* reg)
        : m_id(id), m_reg(reg ? reg : context::get_registry())
    {
    }

    bool gameplay_entity::is_valid() const
    {
        if (!m_reg || m_id == 0)
        {
            return false;
        }
        const auto& entities = m_reg->get_all_entities();
        return entities.find(m_id) != entities.end();
    }

    std::string gameplay_entity::get_name() const
    {
        if (!is_valid())
        {
            return "";
        }
        auto* nameComp = m_reg->get_component<name_component>(m_id);
        return nameComp ? nameComp->name : "";
    }

    void gameplay_entity::set_name(const std::string& name)
    {
        if (!is_valid())
        {
            return;
        }
        auto* nameComp = m_reg->get_component<name_component>(m_id);
        if (nameComp)
        {
            nameComp->name = name;
        }
        else
        {
            m_reg->add_component<name_component>(m_id, name_component{ name });
        }
    }

    transform_wrapper gameplay_entity::transform()
    {
        return transform_wrapper(m_id, m_reg);
    }

    rigidbody_wrapper gameplay_entity::rigidbody()
    {
        return rigidbody_wrapper(m_id, m_reg, context::get_physics_system());
    }

    void gameplay_entity::destroy()
    {
        if (is_valid())
        {
            m_reg->remove_entity(m_id);
            m_id = 0;
        }
    }
}
