/**
 * @file physics_bridge.cpp
 * @brief Fizik dünyası oluşturma, yıkma ve çift yönlü (ECS <-> Jolt) dönüşüm (Transform) eşzamanlama uygulamaları.
 */

#define GLM_ENABLE_EXPERIMENTAL
#include "physics_bridge.h"
#include "../Engine/engine_context.h" 
#include "../Scene/scene.h"
#include "../ECS/components.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp> 

namespace physics_bridge
{
    void create_physics_world(EngineContext& ctx, scene& current_scene)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        auto& body_interface = jolt_system->GetBodyInterface();
        auto& reg = current_scene.get_registry();

        auto& rb_pool = reg.get_all<rigidbody_component>();

        for (auto& [ent, rb] : rb_pool)
        {
            auto* transform = reg.get_component<transform_component>(ent);
            auto* collider = reg.get_component<collider_component>(ent);

            if (!transform || !collider)
            {
                continue;
            }

            JPH::ShapeRefC shape;
            if (collider->type == collider_type::box)
            {
                shape = new JPH::BoxShape(JPH::Vec3(collider->extents.x, collider->extents.y, collider->extents.z));
            }
            else if (collider->type == collider_type::sphere)
            {
                shape = new JPH::SphereShape(collider->extents.x);
            }

            JPH::EMotionType jolt_motion = JPH::EMotionType::Static;
            JPH::ObjectLayer jolt_layer = layers::NON_MOVING;

            if (rb.motion == motion_type::dynamic_body)
            {
                jolt_motion = JPH::EMotionType::Dynamic;
                jolt_layer = layers::MOVING;
            }
            else if (rb.motion == motion_type::kinematic_body)
            {
                jolt_motion = JPH::EMotionType::Kinematic;
                jolt_layer = layers::MOVING;
            }

            // Euler rotasyon değerlerinden dönüşüm matrisi üzerinden güvenli kuaterniyon eldesi (Sıra: X -> Y -> Z)
            glm::mat4 rotMatrix = glm::mat4(1.0f);
            rotMatrix = glm::rotate(rotMatrix, transform->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            rotMatrix = glm::rotate(rotMatrix, transform->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            rotMatrix = glm::rotate(rotMatrix, transform->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

            const glm::quat q = glm::quat_cast(rotMatrix);
            const JPH::Quat jolt_quat(q.x, q.y, q.z, q.w);

            JPH::BodyCreationSettings body_settings(
                shape,
                JPH::RVec3(transform->position.x, transform->position.y, transform->position.z),
                jolt_quat,
                jolt_motion,
                jolt_layer
            );

            body_settings.mFriction = rb.friction;
            body_settings.mRestitution = rb.restitution;

            if (rb.motion == motion_type::dynamic_body)
            {
                body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                body_settings.mMassPropertiesOverride.mMass = rb.mass;
            }

            JPH::EAllowedDOFs allowedDOFs = JPH::EAllowedDOFs::All;
            if (rb.freeze_rotation_x) allowedDOFs &= ~JPH::EAllowedDOFs::RotationX;
            if (rb.freeze_rotation_y) allowedDOFs &= ~JPH::EAllowedDOFs::RotationY;
            if (rb.freeze_rotation_z) allowedDOFs &= ~JPH::EAllowedDOFs::RotationZ;

            body_settings.mAllowedDOFs = allowedDOFs;

            JPH::Body* body = body_interface.CreateBody(body_settings);
            body->SetUserData(static_cast<JPH::uint64>(ent));
            body_interface.AddBody(body->GetID(), JPH::EActivation::Activate);

            rb.body_id = body->GetID();
        }

        jolt_system->OptimizeBroadPhase();
    }

    void sync_physics_to_ecs(EngineContext& ctx, scene& current_scene)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        auto& body_interface = jolt_system->GetBodyInterface();
        auto& reg = current_scene.get_registry();

        auto& rb_pool = reg.get_all<rigidbody_component>();

        for (auto& [ent, rb] : rb_pool)
        {
            if (rb.body_id.IsInvalid()) continue;
            if (rb.motion == motion_type::static_body) continue;

            auto* transform = reg.get_component<transform_component>(ent);
            if (!transform) continue;

            // Konum eşitleme
            const JPH::RVec3 post_sim_pos = body_interface.GetPosition(rb.body_id);
            transform->position = glm::vec3(post_sim_pos.GetX(), post_sim_pos.GetY(), post_sim_pos.GetZ());

            // Rotasyon eşitleme (Render altyapısı ile uyumluluk için güvenli Euler çıkarımı)
            if (rb.motion == motion_type::dynamic_body)
            {
                const JPH::Quat post_sim_quat = body_interface.GetRotation(rb.body_id);

                // GLM Kuaterniyon kurucusu sıralaması: w, x, y, z
                const glm::quat q(post_sim_quat.GetW(), post_sim_quat.GetX(), post_sim_quat.GetY(), post_sim_quat.GetZ());
                const glm::mat4 rotMatrix = glm::mat4_cast(q);

                float rotX, rotY, rotZ;
                // Motorun genel X -> Y -> Z matris kurgusu ile uyumlu ters(çözüm) işlemidir
                glm::extractEulerAngleXYZ(rotMatrix, rotX, rotY, rotZ);

                transform->rotation = glm::vec3(rotX, rotY, rotZ);
            }
        }
    }

    void sync_ecs_to_physics(EngineContext& ctx, scene& current_scene)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        auto& body_interface = jolt_system->GetBodyInterface();
        auto& reg = current_scene.get_registry();

        auto& rb_pool = reg.get_all<rigidbody_component>();
        bool any_static_or_kinematic_moved = false;

        for (auto& [ent, rb] : rb_pool)
        {
            if (rb.body_id.IsInvalid()) continue;
            if (rb.motion == motion_type::kinematic_body) continue;

            auto* transform = reg.get_component<transform_component>(ent);
            if (!transform) continue;

            const JPH::RVec3 jolt_pos = body_interface.GetPosition(rb.body_id);
            const glm::vec3 ecs_pos = transform->position;

            if (glm::distance(glm::vec3(jolt_pos.GetX(), jolt_pos.GetY(), jolt_pos.GetZ()), ecs_pos) > 0.005f)
            {
                glm::mat4 rotMatrix = glm::mat4(1.0f);
                rotMatrix = glm::rotate(rotMatrix, transform->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
                rotMatrix = glm::rotate(rotMatrix, transform->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
                rotMatrix = glm::rotate(rotMatrix, transform->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

                const glm::quat q = glm::quat_cast(rotMatrix);
                const JPH::Quat jolt_quat(q.x, q.y, q.z, q.w);

                body_interface.SetPositionAndRotation(
                    rb.body_id,
                    JPH::RVec3(ecs_pos.x, ecs_pos.y, ecs_pos.z),
                    jolt_quat,
                    JPH::EActivation::Activate
                );

                if (rb.motion == motion_type::static_body || rb.motion == motion_type::kinematic_body)
                {
                    any_static_or_kinematic_moved = true;
                }
            }
        }

        if (any_static_or_kinematic_moved)
        {
            jolt_system->OptimizeBroadPhase();

            for (auto& [ent, rb] : rb_pool)
            {
                if (rb.body_id.IsInvalid()) continue;
                if (rb.motion == motion_type::dynamic_body)
                {
                    body_interface.ActivateBody(rb.body_id);
                }
            }
        }
    }

    void destroy_physics_world(EngineContext& ctx, scene& current_scene)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        auto& body_interface = jolt_system->GetBodyInterface();
        auto& reg = current_scene.get_registry();

        auto& rb_pool = reg.get_all<rigidbody_component>();

        for (auto& [ent, rb] : rb_pool)
        {
            if (rb.body_id.IsInvalid()) continue;

            body_interface.RemoveBody(rb.body_id);
            body_interface.DestroyBody(rb.body_id);
            rb.body_id = JPH::BodyID();
        }
    }
} // namespace physics_bridge