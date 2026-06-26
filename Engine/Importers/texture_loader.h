#pragma once

/**
 * @brief Asset ve Kaynak Yönetimi Modülü
 * * Bu sınıf, disk üzerindeki popüler görsel formatlarını (PNG, JPG, BMP vb.)
 * okuyarak OpenGL doku (texture) nesnelerine dönüştürür.
 */
class texture_loader
{
public:
    /**
     * @brief Belirtilen dosya yolundaki resmi yükler ve GPU belleğine aktarır.
     * * @note PNG formatlarındaki şeffaflık (Alpha) ve resimlerin dikey çevrilme (flip)
     * işlemleri bu fonksiyon içinde otomatik olarak yönetilir.
     * * @param path Yüklenecek görsel dosyasının tam veya göreceli (relative) yolu.
     * @return unsigned int Başarılı ise OpenGL Texture ID (Kimlik Numarası),
     * Başarısız ise hata logu basar ve 0 döner.
     */
    static unsigned int load(const char* path);
};