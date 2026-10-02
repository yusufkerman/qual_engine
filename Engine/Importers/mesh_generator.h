#pragma once

/**
 * @file mesh_generator.h
 * @brief Temel 3B geometrilerin (Küp, Küre vb.) prosedürel olarak üretilmesini sağlayan yardımcı sınıf ve yapıları tanımlar.
 */

#include <vector>
#include <glm/glm.hpp>

 /**
  * @struct generator_vertex
  * @brief Prosedürel ağ üretiminde kullanılan temel tepe noktası verisi.
  */
struct generator_vertex {
    glm::vec3 position;   ///< Yerel uzaydaki 3B tepe noktası konumu.
    glm::vec3 normal;     ///< Yüzey aydınlatması için normal birim vektörü.
    glm::vec2 tex_coords; ///< Doku kaplama için UV koordinatları.
};

/**
 * @struct generated_mesh_data
 * @brief Prosedürel olarak üretilen tepe noktası ve indis tamponlarını barındıran veri yapısı.
 */
struct generated_mesh_data {
    std::vector<generator_vertex> vertices; ///< Üretilen tepe noktalarının listesi.
    std::vector<unsigned int> indices;      ///< Üçgen yüzeyleri tanımlayan indis dizisi.
};

namespace mesh_generator
{
    /**
     * @brief 1x1x1 boyutlarında, merkezi (0, 0, 0) olan standart bir küp ağı üretir.
     * @return generated_mesh_data 24 tepe noktası ve 36 indisten oluşan ağ verisi.
     */
    generated_mesh_data generate_cube();

    /**
     * @brief Belirtilen yarıçap ve çözünürlük değerlerine göre UV küresi (Sphere) üretir.
     *
     * @param radius Kürenin yarıçapı.
     * @param sector_count Boylam dilim sayısı (detay kalitesi).
     * @param stack_count Enlem dilim sayısı (detay kalitesi).
     * @return generated_mesh_data Üretilen küre ağ verisi.
     */
    generated_mesh_data generate_sphere(float radius = 1.0f, unsigned int sector_count = 36, unsigned int stack_count = 18);
}