/**
 * @file collision_event_queue.cpp
 * @brief İş parçacığı güvenli çarpışma olay kuyruğu uygulaması.
 */

#include "collision_event_queue.h"

namespace gameplay
{
    void collision_event_queue::push_event(const pending_collision_event& ev)
    {
        // Geçersiz veya tanımsız varlık eşleşmelerini filtrele
        if (ev.entity_a == 0 || ev.entity_b == 0)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);
        m_events.push_back(ev);
    }

    void collision_event_queue::drain(std::vector<pending_collision_event>& out_events)
    {
        out_events.clear();
        std::lock_guard<std::mutex> lock(m_mutex);
        out_events.swap(m_events);
    }

    void collision_event_queue::clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_events.clear();
    }

    size_t collision_event_queue::size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_events.size();
    }

    bool collision_event_queue::empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_events.empty();
    }
}

