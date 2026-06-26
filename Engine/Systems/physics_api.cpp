#include "physics_api.h"
#include "../Systems/physics_system.h" 

#include <Jolt/Jolt.h>


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
