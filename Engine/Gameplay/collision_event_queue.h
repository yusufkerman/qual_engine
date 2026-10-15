#pragma once

/**
 * @file collision_event_queue.h
 * @brief Jolt fizik iş parçacıkları ile ana oyun döngüsü arasında güvenli veri iletimi sağlayan olay kuyruğu.
 */

#include <vector>
#include <mutex>
#include "collision_types.h"

namespace gameplay
{
    /**
     * @class collision_event_queue
     * @brief Fizik simülasyonu esnasında oluşan temas olaylarını iş parçacığı güvenli (thread-safe) şekilde toplar.
     */
    class collision_event_queue
    {
    public:
        collision_event_queue() = default;
        ~collision_event_queue() = default;

        // Kopyalama ve taşımayı devre dışı bırak
        collision_event_queue(const collision_event_queue&) = delete;
        collision_event_queue& operator=(const collision_event_queue&) = delete;

        /**
         * @brief Fizik dinleyicisinden gelen yeni bir temas olayını kuyruğa ekler.
         * @note Bu fonksiyon çoklu iş parçacığı ortamında (Jolt worker thread) güvenle çağrılabilir.
         * @param ev Kuyruğa eklenecek temas olayı.
         */
        void push_event(const pending_collision_event& ev);

        /**
         * @brief Kuyrukta biriken tüm olayları ana iş parçacığına aktarır ve iç kuyruğu sıfırlar.
         * @param out_events Çekilen olayların doldurulacağı hedef vektör.
         */
        void drain(std::vector<pending_collision_event>& out_events);

        /**
         * @brief Kuyruktaki tüm olayları temizler.
         */
        void clear();

        /**
         * @brief Kuyruktaki bekleyen anlık olay sayısını döner.
         */
        [[nodiscard]] size_t size() const;

        /**
         * @brief Kuyruğun boş olup olmadığını kontrol eder.
         */
        [[nodiscard]] bool empty() const;

    private:
        mutable std::mutex m_mutex;
        std::vector<pending_collision_event> m_events;
    };
}

