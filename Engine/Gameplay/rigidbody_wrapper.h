#pragma once

/**
 * @file rigidbody_wrapper.h
 * @brief Varlıkların fiziksel hareket, hız, kütle ve kuvvet işlemlerini yöneten ergonomik sarmalayıcı.
 */

#include <glm/glm.hpp>
#include "../ECS/entity.h"

class registry;
class physics_system;
struct rigidbody_component;
enum class motion_type;

namespace gameplay
{
    /**
     * @class rigidbody_wrapper
     * @brief Bir varlığın rigidbody_component bileşeni ve Jolt fizik gövdesi üzerinde işlem yapmayı kolaylaştırır.
     */
    class rigidbody_wrapper
    {
    public:
        /**
         * @brief Sarmalayıcıyı varlık, kayıt defteri ve fizik sistemi ile ilklendirir.
         * @param e Bağlı varlık kimliği.
         * @param reg Sahne kayıt defteri.
         * @param phys Fizik sistemi referansı (Opsiyonel, null ise context'ten alınır).
         */
        rigidbody_wrapper(entity e, registry* reg, physics_system* phys = nullptr);

        /**
         * @brief Varlığın geçerli bir rigidbody bileşenine sahip olup olmadığını döner.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Gövdenin anlık doğrusal hız (linear velocity) vektörünü döner.
         */
        [[nodiscard]] glm::vec3 get_velocity() const;

        /**
         * @brief Gövdenin doğrusal hızını ayarlar.
         * @param vel Yeni hız vektörü.
         */
        void set_velocity(const glm::vec3& vel);

        /**
         * @brief Gövdeye ani bir itme (impulse) uygular (ör. Zıplama, darbe alma).
         * @param impulse Uygulanacak itme vektörü.
         */
        void add_impulse(const glm::vec3& impulse);

        /**
         * @brief Gövdeye sürekli bir kuvvet (force) ekler.
         * @param force Uygulanacak kuvvet vektörü.
         */
        void add_force(const glm::vec3& force);

        /**
         * @brief Gövdenin kütlesini (kg) döner.
         */
        [[nodiscard]] float get_mass() const;

        /**
         * @brief Gövdenin kütlesini ayarlar.
         * @param mass Yeni kütle değeri.
         */
        void set_mass(float mass);

        /**
         * @brief Gövdenin sürtünme katsayısını döner.
         */
        [[nodiscard]] float get_friction() const;

        /**
         * @brief Gövdenin sürtünme katsayısını ayarlar.
         * @param friction Yeni sürtünme katsayısı.
         */
        void set_friction(float friction);

        /**
         * @brief Gövdenin esneklik/zıplama katsayısını (restitution) döner.
         */
        [[nodiscard]] float get_restitution() const;

        /**
         * @brief Gövdenin esneklik/zıplama katsayısını ayarlar.
         * @param restitution Yeni esneklik katsayısı (0.0 = sekmez, 1.0 = tam seker).
         */
        void set_restitution(float restitution);

        /**
         * @brief Gövdenin dinamik (fizikten tam etkilenen) olup olmadığını sorgular.
         */
        [[nodiscard]] bool is_dynamic() const;

        /**
         * @brief Gövdenin kinematik (yalnızca kodla hareket eden) olup olmadığını sorgular.
         */
        [[nodiscard]] bool is_kinematic() const;

        /**
         * @brief Gövdenin statik (hareketsiz çevre nesnesi) olup olmadığını sorgular.
         */
        [[nodiscard]] bool is_static() const;

        /**
         * @brief Ham rigidbody_component işaretçisine erişim sağlar.
         */
        [[nodiscard]] rigidbody_component* raw();

    private:
        entity m_entity = 0;
        registry* m_reg = nullptr;
        physics_system* m_phys = nullptr;

        [[nodiscard]] physics_system* resolve_physics_system() const;
    };
}

