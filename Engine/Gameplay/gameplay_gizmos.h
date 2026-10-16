#pragma once

/**
 * @file gameplay_gizmos.h
 * @brief Oyun içi betiklerin (Scripts) çalışma zamanında hata ayıklama şekilleri (çizgi, ışın, küp, küre, çember, ok, koni) çizebilmesini sağlayan Gameplay API servisi.
 */

#include <glm/glm.hpp>

namespace gameplay
{
    /**
     * @class gizmos
     * @brief Çalışma zamanında (Play modunda) görsel hata ayıklama (debug drawing) yapmak için statik API.
     */
    class gizmos
    {
    public:
        /**
         * @brief İki 3B nokta arasına renkli çizgi çizer.
         */
        static void draw_line(
            const glm::vec3& start,
            const glm::vec3& end,
            const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

        /**
         * @brief Başlangıç noktasından verilen yön vektörü boyunca ışın çizer.
         */
        static void draw_ray(
            const glm::vec3& origin,
            const glm::vec3& direction,
            const glm::vec4& color = glm::vec4(1.0f, 0.9f, 0.2f, 1.0f));

        /**
         * @brief Başlangıç noktasından bitiş noktasına doğru ok başlı vektör çizer.
         */
        static void draw_arrow(
            const glm::vec3& start,
            const glm::vec3& end,
            float head_size = 0.25f,
            const glm::vec4& color = glm::vec4(0.2f, 0.8f, 1.0f, 1.0f));

        /**
         * @brief Dünya eksenlerine hizalı tel kafes kutu (Wire Cube) çizer.
         */
        static void draw_wire_cube(
            const glm::vec3& center,
            const glm::vec3& size,
            const glm::vec4& color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

        /**
         * @brief 3 eksenli tel kafes küre (Wire Sphere) çizer.
         */
        static void draw_wire_sphere(
            const glm::vec3& center,
            float radius,
            const glm::vec4& color = glm::vec4(0.2f, 0.85f, 1.0f, 1.0f));

        /**
         * @brief Verilen normal vektörüne dik düzlemde tel kafes çember çizer.
         */
        static void draw_wire_circle(
            const glm::vec3& center,
            const glm::vec3& normal,
            float radius,
            const glm::vec4& color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f),
            int segments = 36);

        /**
         * @brief Verilen tepe noktası ve yönde tel kafes koni (görüş konisi / spot) çizer.
         */
        static void draw_wire_cone(
            const glm::vec3& apex,
            const glm::vec3& direction,
            float range,
            float half_angle_deg,
            const glm::vec4& color = glm::vec4(1.0f, 0.6f, 0.2f, 1.0f));
    };
}

