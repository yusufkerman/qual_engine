/**
 * @file texture_loader.cpp
 * @brief 2B doku yükleme, piksel formatı çözümleme ve GPU parametre yapılandırma uygulaması.
 */

#include "texture_loader.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>
#include "../Externals/stb/stb_image.h"

unsigned int texture_loader::load(const char* path)
{
    if (!path || path[0] == '\0') {
        std::cerr << "[TextureLoader] Gecersiz dosya yolu.\n";
        return 0;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    // OpenGL doku koordinat sistemine (UV sol-alt orijinli) uyum için dikey çevir
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

    if (!data)
    {
        std::cerr << "[TextureLoader] Dosya yuklenemedi: " << path << '\n';
        return 0;
    }

    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat = GL_RGB;

    if (channels == 1)
    {
        internalFormat = GL_R8;
        dataFormat = GL_RED;
    }
    else if (channels == 3)
    {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    }

    unsigned int texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // 1 veya 3 kanallı görsellerde satır sonu bayt hizalama bozulmalarını engelle
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        width,
        height,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data
    );

    // Uzak mesafeler için mipmap katmanlarını otomatik üret
    glGenerateMipmap(GL_TEXTURE_2D);

    // Doku sarma ve filtreleme parametreleri
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Anizotropik filtreleme desteğini sorgula ve uygula
    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);

    if (glGetError() == GL_NO_ERROR && maxAniso > 0.0f)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(8.0f, maxAniso));
    }

    // CPU tarafındaki ham piksel tamponunu serbest bırak
    stbi_image_free(data);

    return texID;
}