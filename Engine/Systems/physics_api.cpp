/**
 * @file physics_api.cpp
 * @brief Varlıklar ile Jolt Physics (BodyInterface) arasındaki doğrudan etkileşim komutlarının uygulanması.
 */

#include "physics_api.h"
#include "../Systems/physics_system.h"
#include <Jolt/Jolt.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include "../ECS/components.h"

physics_api::physics_api(physics_system* phys, registry* reg)
    : m_phys(phys), m_reg(reg)
{
}

void physics_api::set_velocity(entity e, const glm::vec3& v)
{
    if (!this || !m_reg || !m_phys || !m_phys->get_jolt_system())
        return;

    const auto* rb = m_reg->get_component<rigidbody_component>(e);
    if (!rb || rb->body_id.IsInvalid())
    {
        return;
    }

    const JPH::BodyID id = rb->body_id;
    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();
    bodyInterface.SetLinearVelocity(id, JPH::Vec3(v.x, v.y, v.z));
}

glm::vec3 physics_api::get_velocity(entity e)
{
    if (!this || !m_reg || !m_phys || !m_phys->get_jolt_system())
        return glm::vec3(0.0f);

    const auto* rb = m_reg->get_component<rigidbody_component>(e);
    if (!rb || rb->body_id.IsInvalid())
    {
        return glm::vec3(0.0f);
    }

    const JPH::BodyID id = rb->body_id;
    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();
    const JPH::Vec3 vel = bodyInterface.GetLinearVelocity(id);

    return glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());
}

void physics_api::move_kinematic(entity e, const glm::vec3& targetPos, float dt)
{
    if (!this || !m_reg || !m_phys || !m_phys->get_jolt_system())
        return;

    const auto* rb = m_reg->get_component<rigidbody_component>(e);
    if (!rb || rb->body_id.IsInvalid())
    {
        return;
    }

    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();
    bodyInterface.MoveKinematic(
        rb->body_id,
        JPH::Vec3(targetPos.x, targetPos.y, targetPos.z),
        JPH::Quat::sIdentity(),
        dt
    );
}

glm::mat4 physics_api::get_jolt_transform(entity e)
{
    const auto* rb = m_reg->get_component<rigidbody_component>(e);

    if (!rb || rb->body_id.IsInvalid())
    {
        return glm::mat4(1.0f);
    }

    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();

    JPH::RVec3 joltPos;
    JPH::Quat joltRot;
    bodyInterface.GetPositionAndRotation(rb->body_id, joltPos, joltRot);

    const glm::vec3 pos = glm::vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());

    glm::quat rot;
    rot.w = joltRot.GetW();
    rot.x = joltRot.GetX();
    rot.y = joltRot.GetY();
    rot.z = joltRot.GetZ();

    // Dönüşüm işlemleri doğrudan quaternion üzerinden uygulanır.
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), pos);
    modelMatrix = modelMatrix * glm::mat4_cast(rot);

    return modelMatrix;
}

void physics_api::destroy_body(entity e)
{
    auto* rb = m_reg->get_component<rigidbody_component>(e);
    if (!rb || rb->body_id.IsInvalid())
    {
        return;
    }

    // Gövde Jolt dünyasından tamamen koparılır ve imha edilir.
    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();
    bodyInterface.RemoveBody(rb->body_id);
    bodyInterface.DestroyBody(rb->body_id);

    // Tekrar erişimi önlemek için yerel BodyID sıfırlanır.
    rb->body_id = JPH::BodyID();
}

void physics_api::remove_collider_shape(entity e)
{
    const auto* rb = m_reg->get_component<rigidbody_component>(e);

    if (!rb || rb->body_id.IsInvalid())
    {
        return;
    }

    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();

    // Gövdenin çarpışma hacmi (Shape) nullptr atanarak kaldırılır.
    // JPH::EActivation::Activate kullanılarak motorun kütle merkezi ve uyanıklık durumunu güncellemesi zorlanır.
    bodyInterface.SetShape(
        rb->body_id,
        nullptr,
        true,
        JPH::EActivation::Activate
    );
}