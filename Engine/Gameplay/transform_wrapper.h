#pragma once

/**
 * @file transform_wrapper.h
 * @brief Varlıkların uzamsal konum, rotasyon ve yön vektörlerine erişim sağlayan ergonomik sarmalayıcı.
 */

#include <glm/glm.hpp>
#include "../ECS/entity.h"

class registry;
struct transform_component;

namespace gameplay
{
    /**
     * @class transform_wrapper
     * @brief Bir varlığın transform_component bileşenini sararak yön vektörleri, öteleme ve döndürme işlemleri sunar.
     */
    class transform_wrapper
    {
    public:
        /**
         * @brief Belirtilen varlık ve kayıt defteri ile sarmalayıcıyı oluşturur.
         * @param e Bağlı varlık kimliği.
         * @param reg Sahne kayıt defteri.
         */
        transform_wrapper(entity e, registry* reg);

        /**
         * @brief Sarmalayıcının geçerli bir varlığa ve transform bileşenine bağlı olup olmadığını doğrular.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Varlığın dünya/yerel uzaydaki konumunu döner.
         */
        [[nodiscard]] glm::vec3 get_position() const;

        /**
         * @brief Varlığın konumunu günceller.
         * @param pos Yeni konum vektörü.
         */
        void set_position(const glm::vec3& pos);

        /**
         * @brief Varlığın Euler açıları cinsinden rotasyonunu (radyan) döner.
         */
        [[nodiscard]] glm::vec3 get_rotation_euler() const;

        /**
         * @brief Varlığın Euler rotasyonunu günceller.
         * @param euler_rad Radyan cinsinden (Pitch/X, Yaw/Y, Roll/Z) rotasyon.
         */
        void set_rotation_euler(const glm::vec3& euler_rad);

        /**
         * @brief Varlığın ölçeğini (scale) döner.
         */
        [[nodiscard]] glm::vec3 get_scale() const;

        /**
         * @brief Varlığın ölçeğini günceller.
         * @param scale Yeni ölçek vektörü.
         */
        void set_scale(const glm::vec3& scale);

        /**
         * @brief Varlığı belirtilen delta vektörü kadar öteler (yer değiştirir).
         * @param delta Konum farkı.
         */
        void translate(const glm::vec3& delta);

        /**
         * @brief Varlığı belirtilen Euler açı farkı kadar döndürür.
         * @param delta_euler_rad Radyan cinsinden rotasyon artışı.
         */
        void rotate_euler(const glm::vec3& delta_euler_rad);

        /**
         * @brief Varlığın baktığı ileri yön vektörünü döner (Normalize edilmiştir).
         */
        [[nodiscard]] glm::vec3 forward() const;

        /**
         * @brief Varlığın sağ yön vektörünü döner (Normalize edilmiştir).
         */
        [[nodiscard]] glm::vec3 right() const;

        /**
         * @brief Varlığın yukarı yön vektörünü döner (Normalize edilmiştir).
         */
        [[nodiscard]] glm::vec3 up() const;

        /**
         * @brief Varlığı belirtilen hedef noktaya bakacak şekilde yönlendirir.
         * @param target_point Bakılacak hedef dünya koordinatı.
         * @param world_up Yukarı referans ekseni (Varsayılan: +Y).
         */
        void look_at(const glm::vec3& target_point, const glm::vec3& world_up = glm::vec3(0.0f, 1.0f, 0.0f));

        /**
         * @brief Ham transform_component işaretçisine doğrudan erişim sağlar.
         */
        [[nodiscard]] transform_component* raw();

    private:
        entity m_entity = 0;
        registry* m_reg = nullptr;
    };
}

