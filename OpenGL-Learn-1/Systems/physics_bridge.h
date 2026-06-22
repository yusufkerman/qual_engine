#pragma once

class scene;
struct EngineContext;

namespace physics_bridge
{
    // PLAY moduna girildiğinde çağrılır: ECS'deki rigidbodies ve colliders'a bakarak Jolt Body'lerini üretir.
    void create_physics_world(EngineContext& ctx, scene& current_scene);

    // Simülasyon çalışırken her kare çağrılır: Jolt'taki pozisyonları ECS transform_component'e aktarır.
    void sync_physics_to_ecs(EngineContext& ctx, scene& current_scene);

    void sync_ecs_to_physics(EngineContext& ctx, scene& current_scene);

    // STOP moduna girildiğinde çağrılır: Jolt dünyasındaki tüm nesneleri siler ve temizler.
    void destroy_physics_world(EngineContext& ctx, scene& current_scene);
}