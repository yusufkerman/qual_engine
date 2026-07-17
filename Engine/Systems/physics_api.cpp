#include "physics_api.h"
#include "../Systems/physics_system.h" 

#include <Jolt/Jolt.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // translate için
#include <glm/gtc/quaternion.hpp>       // glm::quat ve mat4_cast için

#include <iostream>
#include "../ECS/components.h"


physics_api::physics_api(physics_system* phys, registry* reg)
	: m_phys(phys), m_reg(reg)
{}

void physics_api::set_velocity(entity e, const glm::vec3& v)
{
	rigidbody_component* rb = m_reg->get_component<rigidbody_component>(e);

	if (!rb) {
		std::cout << "[PhysicsAPI] Rigidbody not found" << std::endl;
		return;
	}

    JPH::BodyID id = rb->body_id;
	auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();

	bodyInterface.SetLinearVelocity(id, JPH::Vec3(v.x, v.y, v.z));
}
glm::vec3 physics_api::get_velocity(entity e) 
{
    rigidbody_component* rb = m_reg->get_component<rigidbody_component>(e);

    if (!rb) {
        std::cout << "[PhysicsAPI] Rigidbody not found" << std::endl;
        return glm::vec3(0,0,0);
    }

    JPH::BodyID id = rb->body_id;
    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();

	JPH::Vec3 vel = bodyInterface.GetLinearVelocity(id);

    return glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());
}
void physics_api::move_kinematic(entity e, const glm::vec3& targetPos, float dt)
{
    rigidbody_component* rb = m_reg->get_component<rigidbody_component>(e);
    if (!rb) return;

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
    rigidbody_component* rb = m_reg->get_component<rigidbody_component>(e);

    if (!rb || rb->body_id.IsInvalid()) {
        return glm::mat4(1.0f);
    }

    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();

    JPH::RVec3 joltPos;
    JPH::Quat joltRot;
    bodyInterface.GetPositionAndRotation(rb->body_id, joltPos, joltRot);

    glm::vec3 pos = glm::vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());

    glm::quat rot;
    rot.w = joltRot.GetW();
    rot.x = joltRot.GetX();
    rot.y = joltRot.GetY();
    rot.z = joltRot.GetZ();

    // Euler'a çevirme — direkt quaternion kullan
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), pos);
    modelMatrix = modelMatrix * glm::mat4_cast(rot);

    return modelMatrix;
}

void physics_api::destroy_body(entity e)
{
    rigidbody_component* rb = m_reg->get_component<rigidbody_component>(e);
    if (!rb || rb->body_id.IsInvalid()) return;

    // API motor moduna bakmaz, doğrudan emri uygular!
    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();
    bodyInterface.RemoveBody(rb->body_id);
    bodyInterface.DestroyBody(rb->body_id);

    // Body ID'yi sıfırla
    rb->body_id = JPH::BodyID();
}
void physics_api::remove_collider_shape(entity e)
{
    rigidbody_component* rb = m_reg->get_component<rigidbody_component>(e);

    // Eğer Rigidbody yoksa veya Jolt dünyasında karşılığı yoksa işlem yapmaya gerek yok
    if (!rb || rb->body_id.IsInvalid()) return;

    auto& bodyInterface = m_phys->get_jolt_system()->GetBodyInterface();

    // 🎯 Jolt Dünyasında Şekli Sıfırlama:
    // nullptr geçerek cismin çarpışma hacmini (Shape) kaldırıyoruz. 
    // Sondaki parametreler: Nesneyi uyar (Activate) ve kütle merkezini/inertia ayarlarını koru/güncelle.
    bodyInterface.SetShape(
        rb->body_id,
        nullptr,
        true,
        JPH::EActivation::Activate
    );
}