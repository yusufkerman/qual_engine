#pragma once

/**
 * @file texture_loader.h
 * @brief Disk üzerindeki 2B görsel dosyalarını (PNG, JPG, BMP vb.) okuyup OpenGL dokularına dönüştüren yükleyici.
 */

#include <string>

 /**
  * @class texture_loader
  * @brief Görselleri GPU belleğine aktarıp OpenGL doku tanıtıcısı (Texture ID) üreten statik yardımcı sınıf.
  */
class texture_loader
{
public:
    /**
     * @brief Belirtilen dosya yolundaki resmi yükler, GPU belleğine aktarır ve mipmap oluşturur.
     *
     * @param path Yüklenecek görsel dosyasının tam veya göreli yolu.
     * @return unsigned int Başarılı ise geçerli OpenGL Doku ID'si, başarısızlık durumunda 0 döner.
     */
    static unsigned int load(const char* path);

    /**
     * @brief std::string aşırı yüklemesi (overload).
     * @param path Yüklenecek görsel dosyasının tam veya göreli yolu.
     * @return unsigned int Başarılı ise geçerli OpenGL Doku ID'si, başarısızlık durumunda 0 döner.
     */
    static unsigned int load(const std::string& path) {
        return load(path.c_str());
    }
};