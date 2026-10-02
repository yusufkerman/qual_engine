#pragma once

/**
 * @file cubemap_loader.h
 * @brief OpenGL küp haritası (Cubemap / Skybox) doku yükleme yardımcılarını tanımlar.
 */

#include <string>
#include <vector>

namespace cubemap_loader
{
    /**
     * @brief 6 ayrı görsel dosyasını (+X, -X, +Y, -Y, +Z, -Z sırasıyla) tek bir küp haritasına yükler.
     *
     * @param faces 6 adet dosya yolunu içeren liste.
     * @return unsigned int Oluşturulan OpenGL doku tanıtıcısı (Texture ID), hata durumunda 0.
     */
    unsigned int load_faces(const std::vector<std::string>& faces);

    /**
     * @brief 4x3 yatay haç (horizontal cross) düzenindeki tek bir görsel atlasından küp haritası üretir.
     *
     * @param path Atlas dosyasının yolu.
     * @return unsigned int Oluşturulan OpenGL doku tanıtıcısı (Texture ID), hata durumunda 0.
     */
    unsigned int load_atlas(const std::string& path);
}