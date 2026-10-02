#pragma once

/**
 * @file ModelLoader.h
 * @brief Assimp kütüphanesini kullanarak disk üzerindeki 3B modelleri (.obj, .fbx, .gltf vb.) yükleyen sınıf.
 */

#include "../Graphics/model.h"
#include <string>

 /**
  * @class ModelLoader
  * @brief Harici 3B model dosyalarını okuyarak motorun dahili Model/Mesh yapılarına dönüştüren statik yükleyici.
  */
class ModelLoader
{
public:
    /**
     * @brief Belirtilen dosya yolundaki 3B modeli hiyerarşik olarak belleğe yükler.
     *
     * @details Model yüklenirken poligonlar üçgenleştirilir, normaller ve teğet vektörleri
     * otomatik hesaplanır, mükerrer tepe noktaları birleştirilir ve çevreleme hacmi (Bounding Sphere) çıkarılır.
     *
     * @param path Yüklenecek model dosyasının fiziksel yolu.
     * @return Model Başarılı ise alt ağları (Mesh) doldurulmuş model nesnesi; başarısızlık durumunda boş model nesnesi döner.
     */
    static Model Load(const std::string& path);
};