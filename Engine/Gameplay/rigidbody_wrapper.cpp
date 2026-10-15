/**
 * @file rigidbody_wrapper.cpp
 * @brief Rigidbody ergonomik sarmalayıcı uygulaması.
 */

#include "rigidbody_wrapper.h"
#include "gameplay_context.h"
#include "../ECS/components.h"
#include "../ECS/registry.h"
#include "../Systems/physics_system.h"
#include "../Systems/physics_api.h"

namespace gameplay
{
    rigidbody_wrapper::rigidbody_wrapper(entity e, registry* reg, physics_system* phys)
        : m_entity(e), m_reg(reg), m_phys(phys)
    {
    }

    physics_system* rigidbody_wrapper::resolve_physics_system() const
    {
        if (m_phys)
        {
            return m_phys;
        }
        return context::get_physics_system();
    }

    bool rigidbody_wrapper::is_valid() const
    {
        return m_reg != nullptr && m_entity != 0 && m_reg->has_component<rigidbody_component>(m_entity);
    }

    rigidbody_component* rigidbody_wrapper::raw()
    {
        if (!m_reg || m_entity == 0)
        {
            return nullptr;
        }
        return m_reg->get_component<rigidbody_component>(m_entity);
    }

    glm::vec3 rigidbody_wrapper::get_velocity() const
    {
        auto* phys = resolve_physics_system();
        if (phys && m_entity != 0)
        {
            return phys->api().get_velocity(m_entity);
        }
        return glm::vec3{ 0.0f };
    }

    void rigidbody_wrapper::set_velocity(const glm::vec3& vel)
    {
        auto* phys = resolve_physics_system();
        if (phys && m_entity != 0)
        {
            phys->api().set_velocity(m_entity, vel);
        }
    }

    void rigidbody_wrapper::add_impulse(const glm::vec3& impulse)
    {
        auto* rb = raw();
        if (!rb)
        {
            return;
        }

        const float m = (rb->mass > 0.0001f) ? rb->mass : 1.0f;
        const glm::vec3 delta_vel = impulse / m;
        set_velocity(get_velocity() + delta_vel);
    }

    void rigidbody_wrapper::add_force(const glm::vec3& force)
    {
        // 60 Hz varsayılan sabit adımda kuvveti delta hıza çevir
        constexpr float default_dt = 1.0f / 60.0f;
        add_impulse(force * default_dt);
    }

    float rigidbody_wrapper::get_mass() const
    {
        if (!m_reg || m_entity == 0)
        {
            return 1.0f;
        }
        auto* rb = m_reg->get_component<rigidbody_component>(m_entity);
        return rb ? rb->mass : 1.0f;
    }

    void rigidbody_wrapper::set_mass(float mass)
    {
        auto* rb = raw();
        if (rb)
        {
            rb->mass = mass;
        }
    }

    float rigidbody_wrapper::get_friction() const
    {
        if (!m_reg || m_entity == 0)
        {
            return 0.5f;
        }
        auto* rb = m_reg->get_component<rigidbody_component>(m_entity);
        return rb ? rb->friction : 0.5f;
    }

    void rigidbody_wrapper::set_friction(float friction)
    {
        auto* rb = raw();
        if (rb)
        {
            rb->friction = friction;
        }
    }

    float rigidbody_wrapper::get_restitution() const
    {
        if (!m_reg || m_entity == 0)
        {
            return 0.1f;
        }
        auto* rb = m_reg->get_component<rigidbody_component>(m_entity);
        return rb ? rb->restitution : 0.1f;
    }

    void rigidbody_wrapper::set_restitution(float restitution)
    {
        auto* rb = raw();
        if (rb)
        {
            rb->restitution = restitution;
        }
    }

    bool rigidbody_wrapper::is_dynamic() const
    {
        if (!m_reg || m_entity == 0)
        {
            return false;
        }
        auto* rb = m_reg->get_component<rigidbody_component>(m_entity);
        return rb && rb->motion == motion_type::dynamic_body;
    }

    bool rigidbody_wrapper::is_kinematic() const
    {
        if (!m_reg || m_entity == 0)
        {
            return false;
        }
        auto* rb = m_reg->get_component<rigidbody_component>(m_entity);
        return rb && rb->motion == motion_type::kinematic_body;
    }

    bool rigidbody_wrapper::is_static() const
    {
        if (!m_reg || m_entity == 0)
        {
            return false;
        }
        auto* rb = m_reg->get_component<rigidbody_component>(m_entity);
        return rb && rb->motion == motion_type::static_body;
    }
}

