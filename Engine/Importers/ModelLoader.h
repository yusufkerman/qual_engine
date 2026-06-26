#pragma once

#include "../Graphics/model.h"
#include <string>

/**
 * @brief 3D Model Dosyaları için Yükleme ve İşleme Sorumlusu (Asset Loader)
 * * @details Bu sınıf, Assimp kütüphanesini kullanarak harici disk üzerindeki modern
 * 3D model formatlarını (.obj, .fbx, .gltf vb.) okur, optimize eder ve motor hiyerarşisine
 * uygun bir 'Model' nesnesine dönüştürür.
 */
class ModelLoader
{
public:
    /**
     * @brief Belirtilen dosya yolundaki 3D modeli hiyerarşik olarak yükler.
     * * @note Fonksiyon arka planda otomatik olarak poligonları üçgenleştirir (Triangulation),
     * eksen hizalı sınırları hesaplar (Bounding Volume) ve yinelenen vertexleri temizler.
     * * @param path Yüklenecek 3D model dosyasının tam veya göreceli (relative) dosya yolu.
     * @return Model Başarılı yükleme durumunda içi doldurulmuş mesh hiyerarşisi,
     * başarısızlık durumunda ise içi boş (hata loglu) bir Model nesnesi döner.
     */
    static Model Load(const std::string& path);
};