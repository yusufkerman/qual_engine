#pragma once

/**
 * @file vertex.h
 * @brief Grafik boru hattı için tepe noktası (vertex) veri yapısını tanımlar.
 */

#include <glm/glm.hpp>

 /**
  * @struct Vertex
  * @brief Geometri verilerinin GPU'ya aktarılmasında kullanılan bellek hizalı tepe noktası yapısı.
  */
struct Vertex
{
    glm::vec3 Position; ///< Tepe noktasının yerel koordinat uzayındaki 3B konumu.
    glm::vec3 Normal;   ///< Yüzey aydınlatması için normal birim vektörü.
    glm::vec2 TexCoord; ///< Doku haritalama için UV koordinatları.
    glm::vec3 Tangent;  ///< Normal haritalama (Normal Mapping) için teğet (tangent) vektörü.
};