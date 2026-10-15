#pragma once

/**
 * @file collision_types.h
 * @brief Gameplay API katmanı için çarpışma, temas ve tetikleyici veri yapıları.
 */

#include <glm/glm.hpp>
#include "../ECS/entity.h"

namespace gameplay
{
    /**
     * @enum collision_event_type
     * @brief Fiziksel temas veya tetikleyici olayının durumunu belirten tipler.
     */
    enum class collision_event_type
    {
        enter,   ///< İlk temas anı (çarpışma başladı).
        persist, ///< Temasın devam ettiği kare (çarpışma sürüyor).
        exit     ///< Temasın sonlandığı an (nesneler birbirinden ayrıldı).
    };

    /**
     * @struct collision_hit
     * @brief İki nesne arasındaki katı (solid) fiziksel çarpışmaya dair detaylı bilgiler.
     */
    struct collision_hit
    {
        entity other_entity = 0;              ///< Çarpışılan diğer varlığın kimliği (entity ID).
        glm::vec3 contact_point{ 0.0f };       ///< Çarpışmanın gerçekleştiği dünya koordinatı.
        glm::vec3 contact_normal{ 0.0f, 1.0f, 0.0f }; ///< Temas yüzeyinin normal vektörü.
        float impulse = 0.0f;                 ///< Çarpışmanın uyguladığı impuls (darbe şiddeti).
        bool is_trigger = false;              ///< Olayın bir tetikleyici (sensor) alanından kaynaklanıp kaynaklanmadığı.
    };

    /**
     * @struct trigger_hit
     * @brief Fiziksel kuvvet üretmeyen, içinden geçilebilen sensör/tetikleyici alan olay verisi.
     */
    struct trigger_hit
    {
        entity other_entity = 0;              ///< Tetikleyici alanına giren veya çıkan diğer varlığın kimliği.
    };

    /**
     * @struct pending_collision_event
     * @brief Fizik iş parçacığı havuzundan ana oyun iş parçacığına taşınmak üzere kuyruklanan ham temas olayı.
     */
    struct pending_collision_event
    {
        entity entity_a = 0;                  ///< İlk varlık kimliği.
        entity entity_b = 0;                  ///< İkinci varlık kimliği.
        collision_event_type type = collision_event_type::enter; ///< Olay tipi (Enter, Persist, Exit).
        glm::vec3 point{ 0.0f };               ///< Temas noktası.
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };  ///< Temas normali.
        float impulse = 0.0f;                 ///< İmpuls değeri.
        bool is_sensor = false;               ///< Nesnelerden en az biri tetikleyici (sensor) mi?
    };
}

