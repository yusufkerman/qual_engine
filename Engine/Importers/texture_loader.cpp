#include "texture_loader.h"

#include <glad/glad.h>
#include <iostream>
#include "../Externals/stb/stb_image.h"
#include <glm/common.hpp>

unsigned int texture_loader::load(const char* path)
{
    int width, height, channels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

    if (!data)
    {
        std::cout << "[TextureLoader] Failed: " << path << std::endl;
        return 0;
    }

    GLenum format = GL_RGB;

    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

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

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        std::cout << "[TextureLoader] Anisotropy query failed, GL error: " << err << std::endl;

    std::cout << "[TextureLoader] maxAniso = " << maxAniso << std::endl;

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, glm::min(8.0f, maxAniso));

    stbi_image_free(data);

    return texID;
}