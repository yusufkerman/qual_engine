#pragma once

/**
 * @file mesh.h
 * @brief Tekil bir 3B ağın (mesh) tepe noktalarını, indekslerini ve çevreleme hacmini tanımlar.
 */

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "vertex.h"

 /**
  * @struct Mesh
  * @brief Çizim komutlarına doğrudan iletilen tepe noktası tamponu, indeks tamponu ve sınır verilerini barındıran yapı.
  */
struct Mesh
{
    std::string name;                   ///< Ağın tanımlayıcı ismi.

    std::vector<Vertex> vertices;       ///< Tepe noktası (vertex) dizisi.
    std::vector<unsigned int> indices;  ///< Üçgen indeks dizisi.

    glm::vec3 localCenter{ 0.0f };      ///< Çevreleyen kürenin (bounding sphere) yerel merkez noktası.
    float     localRadius = 0.0f;       ///< Çevreleyen kürenin yerel yarıçapı.
};