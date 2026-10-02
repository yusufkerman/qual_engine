#pragma once

/**
 * @file light_system.h
 * @brief Sahne ışık verilerini GPU Uniform/Shader Storage tamponlarına uygun düzende çıkaran sistem.
 */

#include <vector>
#include <glm/glm.hpp>
#include "../ECS/entity.h"
#include "../ECS/registry.h"
#include "../ECS/components.h"

 /**
  * @struct light_data
  * @brief GPU shader (std140 düzeni) ile birebir uyumlu 80-byte hizalı ışık veri yapısı.
  */
struct light_data
{
    int       type;        ///< Işık türü (0: Directional, 1: Point, 2: Spot).
    float     _pad0[3];    ///< std140 16-byte hizalama dolgusu (Offset: 0 - 16).

    glm::vec3 position;    ///< Işığın dünya uzayındaki konumu.
    float     range;       ///< Işığın etki yarıçapı (Offset: 16 - 32).

    glm::vec3 direction;   ///< Yönlü ve spot ışıklar için ileri yön vektörü.
    float     intensity;   ///< Işık şiddeti çarpanı (Offset: 32 - 48).

    glm::vec3 color;       ///< Işık renk bileşenleri (RGB).
    float     inner_cone;  ///< Spot ışık iç koni açısı cosinüsü (Offset: 48 - 64).

    float     outer_cone;  ///< Spot ışık dış koni açısı cosinüsü.
    float     _pad1[3];    ///< std140 16-byte hizalama dolgusu (Offset: 64 - 80).
};

static_assert(sizeof(light_data) == 80, "light_data boyutu std140 duzeni icin tam 80 byte olmalidir!");

/**
 * @class light_system
 * @brief ECS kayıt defterinden aktif ışık bileşenlerini toplayıp GPU formatına dönüştüren statik servis.
 */
class light_system
{
public:
    light_system() = delete;

    /**
     * @brief Kayıt defterindeki tüm geçerli ışık bileşenlerini çıkarır ve dönüştürür.
     * @param reg ECS kayıt defteri referansı.
     * @return GPU tamponuna yazılmaya hazır light_data dizisi.
     */
    [[nodiscard]] static std::vector<light_data> extract(const registry& reg);
};