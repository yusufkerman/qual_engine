/**
 * @file texture_manager.cpp
 * @brief [LEGACY] Eski doku yöneticisi yaşam döngüsü uygulaması.
 */

#include "texture_manager.h"
#include <glad/glad.h>
#include <iostream>
#include "../Importers/texture_loader.h"

texture_manager::texture_manager()
    : m_FallbackTextureGLID(0)
{
}

texture_manager::~texture_manager()
{
    if (m_FallbackTextureGLID != 0)
    {
        glDeleteTextures(1, &m_FallbackTextureGLID);
        m_FallbackTextureGLID = 0;
    }
}

void texture_manager::init()
{
    if (m_FallbackTextureGLID != 0)
    {
        return;
    }

    glGenTextures(1, &m_FallbackTextureGLID);
    glBindTexture(GL_TEXTURE_2D, m_FallbackTextureGLID);

    constexpr unsigned char whitePixel[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int texture_manager::load(const std::string& path)
{
    const unsigned int glTexture = texture_loader::load(path.c_str());

    if (glTexture == 0)
    {
        std::cerr << "[TextureManager] Doku yukleme basarisiz: " << path << '\n';
        return 0;
    }

    const unsigned int id = nextID++;
    textures[id] = glTexture;
    texturePaths[id] = path;

    return id;
}

unsigned int texture_manager::get(unsigned int id)
{
    const auto it = textures.find(id);
    if (it == textures.end())
    {
        return m_FallbackTextureGLID;
    }
    return it->second;
}

bool texture_manager::has(unsigned int id) const
{
    return textures.find(id) != textures.end();
}

const std::string& texture_manager::get_path(unsigned int id) const
{
    const auto it = texturePaths.find(id);
    if (it == texturePaths.end())
    {
        static const std::string emptyPath = "None";
        return emptyPath;
    }
    return it->second;
}

void texture_manager::clear()
{
    for (const auto& [id, glTexture] : textures)
    {
        if (glTexture != 0)
        {
            glDeleteTextures(1, &glTexture);
        }
    }

    textures.clear();
    texturePaths.clear();
    nextID = 0;
}