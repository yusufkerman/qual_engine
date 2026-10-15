#pragma once

/**
 * @file jolt_contact_listener.h
 * @brief Jolt fizik motorunun temas olaylarını dinleyerek olay kuyruğuna (collision_event_queue) aktaran dinleyici.
 */

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <unordered_map>
#include <mutex>
#include "collision_event_queue.h"

namespace gameplay
{
    /**
     * @class jolt_contact_listener
     * @brief JPH::ContactListener arayüzünü uygulayarak Jolt iç temaslarını yakalar ve iş parçacığı güvenli kuyruğa yazar.
     */
    class jolt_contact_listener : public JPH::ContactListener
    {
    public:
        /**
         * @brief Dinleyiciyi hedef olay kuyruğu ile başlatır.
         * @param queue Temas olaylarının yönlendirileceği hedef kuyruk işaretçisi.
         */
        explicit jolt_contact_listener(collision_event_queue* queue);
        virtual ~jolt_contact_listener() override = default;

        // JPH::ContactListener arayüzü fonksiyonları
        virtual JPH::ValidateResult OnContactValidate(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            JPH::RVec3Arg inBaseOffset,
            const JPH::CollideShapeResult& inCollisionResult) override;

        virtual void OnContactAdded(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings) override;

        virtual void OnContactPersisted(
            const JPH::Body& inBody1,
            const JPH::Body& inBody2,
            const JPH::ContactManifold& inManifold,
            JPH::ContactSettings& ioSettings) override;

        virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

        /**
         * @brief Simülasyon sıfırlandığında gövde çifti önbelleğini temizler.
         */
        void reset();

    private:
        collision_event_queue* m_queue = nullptr;

        // Jolt OnContactRemoved anında gövdeleri doğrudan okumaya izin vermediği için
        // temas halindeki gövde çiftlerini (BodyID pair -> entity pair) önbellekte tutuyoruz.
        struct body_pair_key
        {
            uint32_t id1 = 0;
            uint32_t id2 = 0;

            bool operator==(const body_pair_key& other) const noexcept
            {
                return id1 == other.id1 && id2 == other.id2;
            }
        };

        struct body_pair_hash
        {
            std::size_t operator()(const body_pair_key& k) const noexcept
            {
                return (static_cast<std::size_t>(k.id1) * 397) ^ static_cast<std::size_t>(k.id2);
            }
        };

        struct entity_pair_info
        {
            entity entity_a = 0;
            entity entity_b = 0;
            bool is_sensor = false;
        };

        std::mutex m_cache_mutex;
        std::unordered_map<body_pair_key, entity_pair_info, body_pair_hash> m_active_pairs;
    };
}

