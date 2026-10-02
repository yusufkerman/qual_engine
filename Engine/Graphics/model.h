#pragma once

/**
 * @file model.h
 * @brief Birden çok alt ağdan (sub-mesh) oluşan 3B model varlığı veri yapısını tanımlar.
 */

#include <vector>
#include "mesh.h"

 /**
  * @struct Model
  * @brief Dosyadan yüklenen veya çalışma anında birleştirilen ağların koleksiyonunu temsil eden yapı.
  */
struct Model
{
    std::vector<Mesh> meshes; ///< Modele ait alt ağların (mesh) listesi.
};