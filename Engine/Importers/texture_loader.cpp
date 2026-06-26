#include "texture_loader.h"

#include <glad/glad.h>
#include <iostream>
#include "../Externals/stb/stb_image.h"
#include <glm/common.hpp>

unsigned int texture_loader::load(const char* path)
{
    int width, height, channels;

    // STB image yüklenirken texture'ı yukarıdan aşağı çevirmek için
    stbi_set_flip_vertically_on_load(true);

    // Dosyadan image data okunuyor (raw pixel data)
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

    // Load başarısızsa 0 dönülür (invalid texture id)
    if (!data)
    {
        std::cout << "[TextureLoader] Failed: " << path << std::endl;
        return 0;
    }

    // Kanal sayısına göre OpenGL format belirleniyor
    GLenum format = GL_RGB;

    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    // OpenGL texture oluşturma
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // GPU'ya texture data upload ediliyor
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    // Mipmap generation (uzak mesafede kaliteyi korur)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture wrapping ayarları (UV dışına taşınca repeat)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Min/mag filter ayarları (zoom in/out kalite davranışı)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // GPU'nun desteklediği maksimum anisotropic filtering değeri okunuyor
    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);

    // OpenGL hata kontrolü (driver uyumsuzluklarına karşı debug amaçlı)
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cout << "[TextureLoader] Anisotropy query failed, GL error: " << err << std::endl;

    // Debug amaçlı max anisotropy değeri loglanır
    std::cout << "[TextureLoader] maxAniso = " << maxAniso << std::endl;

    // Anisotropic filtering uygulanıyor (max 8 ile sınırlandırılmış)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, glm::min(8.0f, maxAniso));

    // CPU memory temizleniyor (stb image free)
    stbi_image_free(data);

    // Texture ID geri döndürülür (renderer tarafı bunu kullanır)
    return texID;
}