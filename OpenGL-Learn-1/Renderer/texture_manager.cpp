#include "texture_manager.h"

#include <glad/glad.h>

#include <iostream>
#include "../Importers/texture_loader.h"

unsigned int texture_manager::load(const std::string& path)
{
    unsigned int glTexture = texture_loader::load(path.c_str());

    if (glTexture == 0)
    {
        std::cout << "[TextureManager] Load failed: " << path << std::endl;
        return 0;
    }

    unsigned int id = nextID++;
    textures[id] = glTexture;

    return id;
}

unsigned int texture_manager::get(unsigned int id)
{
    auto it = textures.find(id);

    if (it == textures.end())
    {
        std::cout << "[TextureManager] Invalid texture ID: " << id << std::endl;
        return 0;
    }

    return it->second;
}