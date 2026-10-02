#pragma once

/**
 * @file physics_bridge.h
 * @brief ECS (Entity Component System) ile Jolt Physics dünyası arasındaki senkronizasyonu yöneten köprü katmanı.
 */

class scene;
struct EngineContext;

namespace physics_bridge
{
    /**
     * @brief Oynatma (PLAY) moduna girildiğinde ECS'deki rigidbody ve collider bileşenlerinden Jolt fizik gövdelerini oluşturur.
     * @param ctx Motor bağlamı (Fizik sistemi erişimi için).
     * @param current_scene Okunacak ve güncellenecek sahne nesnesi.
     */
    void create_physics_world(EngineContext& ctx, scene& current_scene);

    /**
     * @brief Simülasyon adımı sonrasında Jolt fizik dünyasındaki güncel konum ve rotasyonları ECS bileşenlerine (transform_component) aktarır.
     * @param ctx Motor bağlamı.
     * @param current_scene Güncellenecek sahne nesnesi.
     */
    void sync_physics_to_ecs(EngineContext& ctx, scene& current_scene);

    /**
     * @brief Çalışma zamanı öncesinde veya harici script müdahalelerinde ECS'de değişen konumları Jolt fizik dünyasına yansıtır.
     * @param ctx Motor bağlamı.
     * @param current_scene Okunacak sahne nesnesi.
     */
    void sync_ecs_to_physics(EngineContext& ctx, scene& current_scene);

    /**
     * @brief Oynatma (STOP) modundan çıkıldığında Jolt dünyasındaki tüm varlıkları temizler ve bellekten siler.
     * @param ctx Motor bağlamı.
     * @param current_scene Temizlenecek sahne nesnesi.
     */
    void destroy_physics_world(EngineContext& ctx, scene& current_scene);
} // namespace physics_bridge