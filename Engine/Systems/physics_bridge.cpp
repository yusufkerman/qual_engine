// Dosya Yolu: Systems/physics_bridge.cpp
#include "physics_bridge.h"
#include "../Engine/engine_context.h" // EngineContext ve alt sistemler için
#include "../Scene/scene.h"
#include "../ECS/components.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <glm/gtc/quaternion.hpp>

namespace physics_bridge
{
    void create_physics_world(EngineContext& ctx, scene& current_scene)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        auto& body_interface = jolt_system->GetBodyInterface();
        auto& reg = current_scene.get_registry();

        // Sahnede hem rigidbody hem de collider komponenti olan tüm entity'leri geziyoruz
        auto& rb_pool = reg.get_all<rigidbody_component>();

        for (auto& [ent, rb] : rb_pool)
        {
            auto* transform = reg.get_component<transform_component>(ent);
            auto* collider = reg.get_component<collider_component>(ent);

            if (!transform || !collider) continue;

            // 1. Collider Şeklini Oluşturma
            JPH::ShapeRefC shape;
            if (collider->type == collider_type::box)
            {
                // Jolt BoxShape için half-extents (yarıçap) bekler
                shape = new JPH::BoxShape(JPH::Vec3(collider->extents.x, collider->extents.y, collider->extents.z));
            }
            else if (collider->type == collider_type::sphere)
            {
                shape = new JPH::SphereShape(collider->extents.x); // x'i yarıçap kabul etmiştik
            }

            // 2. Motion Type ve Layer Kararı
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

            // Rotasyonu Euler'den Quaternion'a çeviriyoruz (GLM -> Jolt)
            glm::quat q(transform->rotation);
            JPH::Quat jolt_quat(q.x, q.y, q.z, q.w);

            // 3. Jolt Body Ayarlarını Hazırlama
            JPH::BodyCreationSettings body_settings(
                shape,
                JPH::RVec3(transform->position.x, transform->position.y, transform->position.z),
                jolt_quat,
                jolt_motion,
                jolt_layer
            );

            // Materyal / Sürtünme ayarları
            body_settings.mFriction = rb.friction;
            body_settings.mRestitution = rb.restitution;

            if (rb.motion == motion_type::dynamic_body)
            {
                body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                body_settings.mMassPropertiesOverride.mMass = rb.mass;
            }

            // =========================================================
            // CREATION-TIME ROTATION FREEZE
            // =========================================================
            JPH::EAllowedDOFs allowedDOFs = JPH::EAllowedDOFs::All;

            if (rb.freeze_rotation_x) allowedDOFs &= ~JPH::EAllowedDOFs::RotationX;
            if (rb.freeze_rotation_y) allowedDOFs &= ~JPH::EAllowedDOFs::RotationY;
            if (rb.freeze_rotation_z) allowedDOFs &= ~JPH::EAllowedDOFs::RotationZ;

            body_settings.mAllowedDOFs = allowedDOFs;
            // =========================================================

            // 4. Dünyada Nesneyi Yarat ve Aktive Et
            JPH::Body* body = body_interface.CreateBody(body_settings);

            // Jolt Body'sine bizim entity ID'sini veriyoruz ki geri dönüp kim olduğunu bilelim
            body->SetUserData(static_cast<JPH::uint64>(ent));

            body_interface.AddBody(body->GetID(), JPH::EActivation::Activate);

            // Jolt'un verdiği benzersiz ID'yi bizim komponente kaydediyoruz
            rb.body_id = body->GetID();
        }

        // Geniş faz ağaçlarını optimize etmesi için Jolt'u tetikliyoruz
        jolt_system->OptimizeBroadPhase();
    }

    void physics_bridge::sync_physics_to_ecs(EngineContext& ctx, scene& current_scene)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        auto& body_interface = jolt_system->GetBodyInterface();
        auto& reg = current_scene.get_registry();

        auto& rb_pool = reg.get_all<rigidbody_component>();

        for (auto& [ent, rb] : rb_pool)
        {
            if (rb.body_id.IsInvalid()) continue;

            // Static'leri atla, kinematic ve dynamic'leri güncelle
            if (rb.motion == motion_type::static_body)
                continue;

            auto* transform = reg.get_component<transform_component>(ent);
            if (!transform) continue;

            JPH::RVec3 post_sim_pos = body_interface.GetPosition(rb.body_id);
            transform->position = glm::vec3(post_sim_pos.GetX(), post_sim_pos.GetY(), post_sim_pos.GetZ());

            // Kinematic'te rotasyonu Jolt'tan okumana gerek yok, sadece pozisyon yeterli
            if (rb.motion == motion_type::dynamic_body)
            {
                JPH::Quat post_sim_quat = body_interface.GetRotation(rb.body_id);
                glm::quat q(post_sim_quat.GetW(), post_sim_quat.GetX(), post_sim_quat.GetY(), post_sim_quat.GetZ());
                transform->rotation = glm::degrees(glm::eulerAngles(q));
            }
        }
    }
    void physics_bridge::sync_ecs_to_physics(EngineContext& ctx, scene& current_scene)
    {
        auto* jolt_system = ctx.physicsSystem.get_jolt_system();
        auto& body_interface = jolt_system->GetBodyInterface();
        auto& reg = current_scene.get_registry();

        auto& rb_pool = reg.get_all<rigidbody_component>();
        bool any_static_or_kinematic_moved = false;

        for (auto& [ent, rb] : rb_pool)
        {
            if (rb.body_id.IsInvalid()) continue;

            if (rb.motion == motion_type::kinematic_body)
                continue;

            auto* transform = reg.get_component<transform_component>(ent);
            if (!transform) continue;

            JPH::RVec3 jolt_pos = body_interface.GetPosition(rb.body_id);
            glm::vec3 ecs_pos = transform->position;

            // Tolerans kontrolü
            if (glm::distance(glm::vec3(jolt_pos.GetX(), jolt_pos.GetY(), jolt_pos.GetZ()), ecs_pos) > 0.005f)
            {
                glm::quat q(transform->rotation);
                JPH::Quat jolt_quat(q.x, q.y, q.z, q.w);

                // Nesneyi yeni yerine ışınla
                body_interface.SetPositionAndRotation(
                    rb.body_id,
                    JPH::RVec3(ecs_pos.x, ecs_pos.y, ecs_pos.z),
                    jolt_quat,
                    JPH::EActivation::Activate // Dokunulan nesneyi uyandır
                );

                if (rb.motion == motion_type::static_body || rb.motion == motion_type::kinematic_body)
                {
                    any_static_or_kinematic_moved = true;
                }
            }
        }

        // ⚠️ KÜREYİ HAVADA KALMAKTAN KURTARAN SİHİRLİ DOKUNUŞ:
        if (any_static_or_kinematic_moved)
        {
            // 1. Önce Jolt'un haritasını (Broadphase) güncelle ki zemin yeni yerine yerleşsin
            jolt_system->OptimizeBroadPhase();

            // 2. Sahnede uykuda olan NE KADAR NESNE VARSA HEPSİNİ ZORLA UYANDIR!
            // Zemin altlarından kaçtığı için hepsi uyanıp tekrar fizik simülasyonuna girecek.
            for (auto& [ent, rb] : rb_pool)
            {
                if (rb.body_id.IsInvalid()) continue;

                // Sadece dinamik nesneleri (küre gibi) uyandırıyoruz
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

            // Dünyadan kaldır ve hafızasını temizle
            body_interface.RemoveBody(rb.body_id);
            body_interface.DestroyBody(rb.body_id);

            // ID'yi sıfırla
            rb.body_id = JPH::BodyID();
        }
    }
}