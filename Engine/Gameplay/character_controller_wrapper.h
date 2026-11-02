#pragma once

/**
 * @file character_controller_wrapper.h
 * @brief Kinematik karakter kontrolcusu (CharacterVirtual) ergonomik erisim sarmalayicisi.
 */

#include <glm/glm.hpp>
#include "../ECS/entity.h"

class registry;
class physics_system;
struct character_controller_component;

namespace gameplay
{
    /**
     * @class character_controller_wrapper
     * @brief Varlik uzerindeki character_controller_component ve Jolt CharacterVirtual ile kolay iletisim saglar.
     */
    class character_controller_wrapper
    {
    public:
        /**
         * @brief Sarmalayiciyi varlik, kayit defteri ve fizik sistemi ile baslatir.
         * @param e Bagli varlik kimligi.
         * @param reg Sahne kayit defteri.
         * @param phys Fizik sistemi referansi (nullptr ise gameplay context'ten cozulur).
         */
        character_controller_wrapper(entity e, registry* reg, physics_system* phys = nullptr);

        /**
         * @brief Varligin gecerli bir character_controller_component bilesenine sahip olup olmadigini doner.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Ham bilesen isaretcisine erisir.
         */
        [[nodiscard]] character_controller_component* raw();
        [[nodiscard]] const character_controller_component* raw() const;

        /**
         * @brief Karakterin anlik dogrusal hizini (linear velocity) doner.
         */
        [[nodiscard]] glm::vec3 get_velocity() const;

        /**
         * @brief Karakterin dogrusal hizini dogrudan ayarlar.
         * @param vel Yeni hiz vektoru.
         */
        void set_velocity(const glm::vec3& vel);

        /**
         * @brief Karakteri belirtilen yatay/dikey hiz vektoruyle hareket ettirir.
         * @param velocity Uygulanacak hiz (ornegin yurume yonu * hiz).
         */
        void move(const glm::vec3& velocity);

        /**
         * @brief Karakter zemindeyse ziplama hizi kazandirir.
         * @param custom_jump_speed 0 ise bilesendeki jump_speed kullanilir.
         */
        void jump(float custom_jump_speed = 0.0f);

        /**
         * @brief Karakterin anlik olarak zeminde olup olmadigini doner.
         */
        [[nodiscard]] bool is_grounded() const;

        /**
         * @brief Standart yurume hizini doner.
         */
        [[nodiscard]] float get_walk_speed() const;

        /**
         * @brief Standart yurume hizini ayarlar.
         */
        void set_walk_speed(float speed);

        /**
         * @brief Standart ziplama hizini doner.
         */
        [[nodiscard]] float get_jump_speed() const;

        /**
         * @brief Standart ziplama hizini ayarlar.
         */
        void set_jump_speed(float speed);

        /**
         * @brief Karakter uzerindeki yercekimi ivmesini doner.
         */
        [[nodiscard]] float get_gravity() const;

        /**
         * @brief Karakter uzerindeki yercekimi ivmesini ayarlar.
         */
        void set_gravity(float gravity);

        /**
         * @brief Takilmadan tirmanilabilecek basamak yuksekligini doner.
         */
        [[nodiscard]] float get_step_height() const;

        /**
         * @brief Takilmadan tirmanilabilecek basamak yuksekligini ayarlar.
         */
        void set_step_height(float height);

        /**
         * @brief Maksimum egim tirmanma acisini doner (derece).
         */
        [[nodiscard]] float get_max_slope_angle() const;

        /**
         * @brief Maksimum egim tirmanma acisini ayarlar (derece).
         */
        void set_max_slope_angle(float angle_degrees);

    private:
        physics_system* resolve_physics_system() const;

        entity m_entity = 0;
        registry* m_reg = nullptr;
        physics_system* m_phys = nullptr;
    };
} // namespace gameplay

