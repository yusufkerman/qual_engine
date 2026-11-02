/**
 * @file physics_bridge.cpp
 * @brief Fizik dünyası oluşturma, yıkma ve çift yönlü (ECS <-> Jolt) dönüşüm (Transform) eşzamanlama uygulamaları.
 */

#define GLM_ENABLE_EXPERIMENTAL
#include "physics_bridge.h"
#include "../Engine/engine_context.h" 
#include "../Scene/scene.h"
#include "../ECS/components.h"
#include "../ECS/transform_utils.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include "physics_system.h"

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

            const glm::mat4 worldMat = ecs_transform::get_world_matrix(ent, reg);
            const glm::vec3 worldPos = glm::vec3(worldMat[3]);

            glm::vec3 worldScale;
            worldScale.x = glm::length(glm::vec3(worldMat[0]));
            worldScale.y = glm::length(glm::vec3(worldMat[1]));
            worldScale.z = glm::length(glm::vec3(worldMat[2]));
            const glm::vec3 effScale = glm::max(worldScale, glm::vec3(0.001f));

            glm::mat3 rotMat;
            rotMat[0] = glm::vec3(worldMat[0]) / effScale.x;
            rotMat[1] = glm::vec3(worldMat[1]) / effScale.y;
            rotMat[2] = glm::vec3(worldMat[2]) / effScale.z;

            const glm::quat q = glm::quat_cast(rotMat);
            const JPH::Quat jolt_quat(q.x, q.y, q.z, q.w);

            JPH::ShapeRefC shape;
            if (collider->type == collider_type::box)
            {
                // Nesnenin transform scale degeriyle orantili fizik sinirlari
                const glm::vec3 halfExtents = glm::max(collider->extents * effScale, glm::vec3(0.01f));
                shape = new JPH::BoxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
            }
            else if (collider->type == collider_type::sphere)
            {
                const float maxScale = std::max({ effScale.x, effScale.y, effScale.z });
                const float radius = std::max(0.01f, collider->extents.x * maxScale);
                shape = new JPH::SphereShape(radius);
            }

            // Offset uygula: Eger offset sifirdan farkliysa, RotatedTranslatedShape ile sar
            if (collider->offset != glm::vec3(0.0f))
            {
                const glm::vec3 scaledOffset = collider->offset * effScale;
                shape = new JPH::RotatedTranslatedShape(
                    JPH::Vec3(scaledOffset.x, scaledOffset.y, scaledOffset.z),
                    JPH::Quat::sIdentity(),
                    shape
                );
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

            JPH::BodyCreationSettings body_settings(
                shape,
                JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
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

        // 2. Kinematik Karakter Kontrolculeri (CharacterVirtual) Olustur
        auto& cc_pool = reg.get_all<character_controller_component>();
        for (auto& [ent, cc] : cc_pool)
        {
            auto* transform = reg.get_component<transform_component>(ent);
            if (!transform) continue;

            const float radius = (cc.radius > 0.01f) ? cc.radius : 0.4f;
            const float height = (cc.height > radius * 2.0f) ? cc.height : radius * 2.1f;
            const float half_cylinder = (height - 2.0f * radius) * 0.5f;

            JPH::Ref<JPH::Shape> capsule = new JPH::CapsuleShape(half_cylinder, radius);
            JPH::Ref<JPH::RotatedTranslatedShape> shape = new JPH::RotatedTranslatedShape(
                JPH::Vec3(0.0f, half_cylinder + radius, 0.0f),
                JPH::Quat::sIdentity(),
                capsule
            );

            JPH::CharacterVirtualSettings settings;
            settings.mShape = shape;
            settings.mMaxSlopeAngle = JPH::DegreesToRadians(cc.max_slope_angle);
            settings.mUp = JPH::Vec3::sAxisY();
            settings.mMass = 70.0f;

            glm::mat4 rotMatrix = glm::mat4(1.0f);
            rotMatrix = glm::rotate(rotMatrix, transform->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            rotMatrix = glm::rotate(rotMatrix, transform->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            rotMatrix = glm::rotate(rotMatrix, transform->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

            const glm::quat q = glm::quat_cast(rotMatrix);
            const JPH::Quat jolt_quat(q.x, q.y, q.z, q.w);

            cc.character = new JPH::CharacterVirtual(
                &settings,
                JPH::RVec3(transform->position.x, transform->position.y, transform->position.z),
                jolt_quat,
                static_cast<JPH::uint64>(ent), jolt_system); cc.linear_velocity = glm::vec3(0.0f); }

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

            const JPH::RVec3 post_sim_pos = body_interface.GetPosition(rb.body_id);
            const JPH::Quat post_sim_quat = body_interface.GetRotation(rb.body_id);

            auto* hier = reg.get_component<hierarchy_component>(ent);
            if (hier && hier->parent != 0)
            {
                const glm::mat4 parentWorldMat = ecs_transform::get_world_matrix(hier->parent, reg);
                const glm::mat4 invParent = glm::inverse(parentWorldMat);
                const glm::vec3 worldPos(post_sim_pos.GetX(), post_sim_pos.GetY(), post_sim_pos.GetZ());
                transform->position = glm::vec3(invParent * glm::vec4(worldPos, 1.0f));

                if (rb.motion == motion_type::dynamic_body)
                {
                    const glm::quat q(post_sim_quat.GetW(), post_sim_quat.GetX(), post_sim_quat.GetY(), post_sim_quat.GetZ());
                    const glm::mat4 worldRotMat = glm::mat4_cast(q);
                    const glm::mat4 localMat = invParent * (glm::translate(glm::mat4(1.0f), worldPos) * worldRotMat);
                    float rotX, rotY, rotZ;
                    glm::extractEulerAngleXYZ(localMat, rotX, rotY, rotZ);
                    transform->rotation = glm::vec3(rotX, rotY, rotZ);
                }
            }
            else
            {
                transform->position = glm::vec3(post_sim_pos.GetX(), post_sim_pos.GetY(), post_sim_pos.GetZ());
                if (rb.motion == motion_type::dynamic_body)
                {
                    const glm::quat q(post_sim_quat.GetW(), post_sim_quat.GetX(), post_sim_quat.GetY(), post_sim_quat.GetZ());
                    const glm::mat4 rotMatrix = glm::mat4_cast(q);
                    float rotX, rotY, rotZ;
                    glm::extractEulerAngleXYZ(rotMatrix, rotX, rotY, rotZ);
                    transform->rotation = glm::vec3(rotX, rotY, rotZ);
                }
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

            const glm::mat4 worldMat = ecs_transform::get_world_matrix(ent, reg);
            const glm::vec3 ecs_pos = glm::vec3(worldMat[3]);

            glm::vec3 worldScale;
            worldScale.x = glm::length(glm::vec3(worldMat[0]));
            worldScale.y = glm::length(glm::vec3(worldMat[1]));
            worldScale.z = glm::length(glm::vec3(worldMat[2]));
            const glm::vec3 effScale = glm::max(worldScale, glm::vec3(0.001f));

            glm::mat3 rotMat;
            rotMat[0] = glm::vec3(worldMat[0]) / effScale.x;
            rotMat[1] = glm::vec3(worldMat[1]) / effScale.y;
            rotMat[2] = glm::vec3(worldMat[2]) / effScale.z;

            const glm::quat q = glm::quat_cast(rotMat);
            const JPH::Quat jolt_quat(q.x, q.y, q.z, q.w);

            const JPH::RVec3 jolt_pos = body_interface.GetPosition(rb.body_id);
            const JPH::Quat current_jolt_quat = body_interface.GetRotation(rb.body_id);
            const glm::quat current_q(current_jolt_quat.GetW(), current_jolt_quat.GetX(), current_jolt_quat.GetY(), current_jolt_quat.GetZ());

            const bool posChanged = glm::distance(glm::vec3(jolt_pos.GetX(), jolt_pos.GetY(), jolt_pos.GetZ()), ecs_pos) > 0.005f;
            const bool rotChanged = (1.0f - std::abs(glm::dot(q, current_q))) > 0.0001f;

            if (posChanged || rotChanged)
            {
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

    void update_character_controllers(EngineContext& ctx, scene& current_scene, float delta_time)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        if (!jolt_system) return;

        auto& reg = current_scene.get_registry();
        auto& cc_pool = reg.get_all<character_controller_component>();

        JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
        update_settings.mStickToFloorStepDown = JPH::Vec3(0.0f, -0.5f, 0.0f);
        update_settings.mWalkStairsStepUp = JPH::Vec3(0.0f, 0.35f, 0.0f);
        update_settings.mWalkStairsMinStepForward = 0.02f;
        update_settings.mWalkStairsStepForwardTest = 0.15f;

        JPH::DefaultBroadPhaseLayerFilter bp_filter(ctx.physicsSystem.get_object_vs_bp_filter(), layers::MOVING);
        JPH::DefaultObjectLayerFilter obj_filter(ctx.physicsSystem.get_object_layer_pair_filter(), layers::MOVING);
        JPH::BodyFilter body_filter;
        JPH::ShapeFilter shape_filter;
        auto* temp_allocator = ctx.physicsSystem.get_temp_allocator();

        for (auto& [ent, cc] : cc_pool)
        {
            if (!cc.character) continue;
            auto* transform = reg.get_component<transform_component>(ent);
            if (!transform) continue;

            update_settings.mWalkStairsStepUp = JPH::Vec3(0.0f, cc.step_height, 0.0f);

            // Yercekimi ivmesini dikey hiza uygula
            cc.linear_velocity.y += cc.gravity * delta_time;

            // Jolt nesnesine hizi aktar
            cc.character->SetLinearVelocity(JPH::Vec3(cc.linear_velocity.x, cc.linear_velocity.y, cc.linear_velocity.z));

            // Kinematik carpisma, egim ve basamak cozumlemesini yurut
            cc.character->ExtendedUpdate(
                delta_time,
                JPH::Vec3(0.0f, cc.gravity, 0.0f),
                update_settings,
                bp_filter,
                obj_filter,
                body_filter,
                shape_filter,
                *temp_allocator
            );

            // Guncellenmis konumu ECS transform'a yansit
            const JPH::RVec3 new_pos = cc.character->GetPosition();
            transform->position = glm::vec3(new_pos.GetX(), new_pos.GetY(), new_pos.GetZ());

            const JPH::Vec3 new_vel = cc.character->GetLinearVelocity();
            cc.linear_velocity = glm::vec3(new_vel.GetX(), new_vel.GetY(), new_vel.GetZ());

            cc.is_grounded = (cc.character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround);
            if (cc.is_grounded && cc.linear_velocity.y < 0.0f)
            {
                cc.linear_velocity.y = 0.0f;
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

        auto& cc_pool = reg.get_all<character_controller_component>();
        for (auto& [ent, cc] : cc_pool)
        {
            cc.character = nullptr;
        }
    }
} // namespace physics_bridge
