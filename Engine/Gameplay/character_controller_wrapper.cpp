/**
 * @file character_controller_wrapper.cpp
 * @brief Kinematik karakter kontrolcusu sarmalayici uygulamasi.
 */

#include "character_controller_wrapper.h"
#include "gameplay_context.h"
#include "../ECS/components.h"
#include "../ECS/registry.h"
#include "../Systems/physics_system.h"
#include <cmath>

namespace gameplay
{
    character_controller_wrapper::character_controller_wrapper(entity e, registry* reg, physics_system* phys)
        : m_entity(e), m_reg(reg), m_phys(phys)
    {
    }

    physics_system* character_controller_wrapper::resolve_physics_system() const
    {
        if (m_phys)
        {
            return m_phys;
        }
        return context::get_physics_system();
    }

    bool character_controller_wrapper::is_valid() const
    {
        return m_reg != nullptr && m_entity != 0 && m_reg->has_component<character_controller_component>(m_entity);
    }

    character_controller_component* character_controller_wrapper::raw()
    {
        if (!m_reg || m_entity == 0)
        {
            return nullptr;
        }
        return m_reg->get_component<character_controller_component>(m_entity);
    }

    const character_controller_component* character_controller_wrapper::raw() const
    {
        if (!m_reg || m_entity == 0)
        {
            return nullptr;
        }
        return m_reg->get_component<character_controller_component>(m_entity);
    }

    glm::vec3 character_controller_wrapper::get_velocity() const
    {
        auto* cc = raw();
        return cc ? cc->linear_velocity : glm::vec3{ 0.0f };
    }

    void character_controller_wrapper::set_velocity(const glm::vec3& vel)
    {
        auto* cc = raw();
        if (cc)
        {
            cc->linear_velocity = vel;
            if (cc->character)
            {
                cc->character->SetLinearVelocity(JPH::Vec3(vel.x, vel.y, vel.z));
            }
        }
    }

    void character_controller_wrapper::move(const glm::vec3& velocity)
    {
        auto* cc = raw();
        if (!cc) return;

        cc->linear_velocity.x = velocity.x;
        cc->linear_velocity.z = velocity.z;
        if (std::abs(velocity.y) > 0.001f)
        {
            cc->linear_velocity.y = velocity.y;
        }

        if (cc->character)
        {
            cc->character->SetLinearVelocity(JPH::Vec3(cc->linear_velocity.x, cc->linear_velocity.y, cc->linear_velocity.z));
        }
    }

    void character_controller_wrapper::jump(float custom_jump_speed)
    {
        auto* cc = raw();
        if (!cc) return;

        if (cc->is_grounded)
        {
            const float jSpeed = (custom_jump_speed > 0.0f) ? custom_jump_speed : cc->jump_speed;
            cc->linear_velocity.y = jSpeed;
            cc->is_grounded = false;

            if (cc->character)
            {
                cc->character->SetLinearVelocity(JPH::Vec3(cc->linear_velocity.x, cc->linear_velocity.y, cc->linear_velocity.z));
            }
        }
    }

    bool character_controller_wrapper::is_grounded() const
    {
        auto* cc = raw();
        return cc ? cc->is_grounded : false;
    }

    float character_controller_wrapper::get_walk_speed() const
    {
        auto* cc = raw();
        return cc ? cc->walk_speed : 5.0f;
    }

    void character_controller_wrapper::set_walk_speed(float speed)
    {
        auto* cc = raw();
        if (cc) cc->walk_speed = speed;
    }

    float character_controller_wrapper::get_jump_speed() const
    {
        auto* cc = raw();
        return cc ? cc->jump_speed : 5.0f;
    }

    void character_controller_wrapper::set_jump_speed(float speed)
    {
        auto* cc = raw();
        if (cc) cc->jump_speed = speed;
    }

    float character_controller_wrapper::get_gravity() const
    {
        auto* cc = raw();
        return cc ? cc->gravity : -9.81f;
    }

    void character_controller_wrapper::set_gravity(float gravity)
    {
        auto* cc = raw();
        if (cc) cc->gravity = gravity;
    }

    float character_controller_wrapper::get_step_height() const
    {
        auto* cc = raw();
        return cc ? cc->step_height : 0.35f;
    }

    void character_controller_wrapper::set_step_height(float height)
    {
        auto* cc = raw();
        if (cc) cc->step_height = height;
    }

    float character_controller_wrapper::get_max_slope_angle() const
    {
        auto* cc = raw();
        return cc ? cc->max_slope_angle : 45.0f;
    }

    void character_controller_wrapper::set_max_slope_angle(float angle_degrees)
    {
        auto* cc = raw();
        if (cc)
        {
            cc->max_slope_angle = angle_degrees;
            if (cc->character)
            {
                cc->character->SetMaxSlopeAngle(JPH::DegreesToRadians(angle_degrees));
            }
        }
    }
} // namespace gameplay

