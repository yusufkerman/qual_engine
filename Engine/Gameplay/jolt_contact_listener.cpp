/**
 * @file jolt_contact_listener.cpp
 * @brief Jolt temas dinleyicisi uygulaması.
 */

#include "jolt_contact_listener.h"
#include <Jolt/Physics/Body/Body.h>

namespace gameplay
{
    jolt_contact_listener::jolt_contact_listener(collision_event_queue* queue)
        : m_queue(queue)
    {
    }

    JPH::ValidateResult jolt_contact_listener::OnContactValidate(
        const JPH::Body& /*inBody1*/,
        const JPH::Body& /*inBody2*/,
        JPH::RVec3Arg /*inBaseOffset*/,
        const JPH::CollideShapeResult& /*inCollisionResult*/)
    {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void jolt_contact_listener::OnContactAdded(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings)
    {
        const entity ent1 = static_cast<entity>(inBody1.GetUserData());
        const entity ent2 = static_cast<entity>(inBody2.GetUserData());

        if (ent1 == 0 || ent2 == 0)
        {
            return;
        }

        const uint32_t b1 = inBody1.GetID().GetIndexAndSequenceNumber();
        const uint32_t b2 = inBody2.GetID().GetIndexAndSequenceNumber();

        // Çifti önbelleğe al (OnContactRemoved için)
        {
            std::lock_guard<std::mutex> lock(m_cache_mutex);
            m_active_pairs[{b1, b2}] = { ent1, ent2, ioSettings.mIsSensor };
        }

        if (!m_queue)
        {
            return;
        }

        pending_collision_event ev;
        ev.entity_a = ent1;
        ev.entity_b = ent2;
        ev.type = collision_event_type::enter;
        ev.is_sensor = ioSettings.mIsSensor;

        // Temas noktası ve normali
        const JPH::RVec3 pt = inManifold.mBaseOffset;
        ev.point = glm::vec3(pt.GetX(), pt.GetY(), pt.GetZ());
        ev.normal = glm::vec3(inManifold.mWorldSpaceNormal.GetX(),
                              inManifold.mWorldSpaceNormal.GetY(),
                              inManifold.mWorldSpaceNormal.GetZ());
        ev.impulse = inManifold.mPenetrationDepth;

        m_queue->push_event(ev);
    }

    void jolt_contact_listener::OnContactPersisted(
        const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings)
    {
        const entity ent1 = static_cast<entity>(inBody1.GetUserData());
        const entity ent2 = static_cast<entity>(inBody2.GetUserData());

        if (ent1 == 0 || ent2 == 0 || !m_queue)
        {
            return;
        }

        pending_collision_event ev;
        ev.entity_a = ent1;
        ev.entity_b = ent2;
        ev.type = collision_event_type::persist;
        ev.is_sensor = ioSettings.mIsSensor;

        const JPH::RVec3 pt = inManifold.mBaseOffset;
        ev.point = glm::vec3(pt.GetX(), pt.GetY(), pt.GetZ());
        ev.normal = glm::vec3(inManifold.mWorldSpaceNormal.GetX(),
                              inManifold.mWorldSpaceNormal.GetY(),
                              inManifold.mWorldSpaceNormal.GetZ());
        ev.impulse = inManifold.mPenetrationDepth;

        m_queue->push_event(ev);
    }

    void jolt_contact_listener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
    {
        const uint32_t b1 = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
        const uint32_t b2 = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();

        entity_pair_info info;
        bool found = false;

        {
            std::lock_guard<std::mutex> lock(m_cache_mutex);
            auto it = m_active_pairs.find({ b1, b2 });
            if (it != m_active_pairs.end())
            {
                info = it->second;
                found = true;
                m_active_pairs.erase(it);
            }
        }

        if (found && m_queue)
        {
            pending_collision_event ev;
            ev.entity_a = info.entity_a;
            ev.entity_b = info.entity_b;
            ev.type = collision_event_type::exit;
            ev.is_sensor = info.is_sensor;
            m_queue->push_event(ev);
        }
    }

    void jolt_contact_listener::reset()
    {
        std::lock_guard<std::mutex> lock(m_cache_mutex);
        m_active_pairs.clear();
    }
}

