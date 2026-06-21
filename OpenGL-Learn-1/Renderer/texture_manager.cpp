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
    texturePaths[id] = path; // 🔥 KRİTİK: Yüklenen dokunun dosya yolunu kaydettik!

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

bool texture_manager::has(unsigned int id) const
{
    return textures.find(id) != textures.end();
}

// 🔍 Yeni eklenen fonksiyonun gövdesi
const std::string& texture_manager::get_path(unsigned int id) const
{
    auto it = texturePaths.find(id);
    if (it == texturePaths.end())
    {
        static std::string emptyPath = "None";
        return emptyPath;
    }
    return it->second;
}
void texture_manager::clear()
{
    // 1. Haritadaki tüm OpenGL doku ID'lerini ekran kartından siliyoruz
    for (const auto& [id, glTexture] : textures)
    {
        if (glTexture != 0)
        {
            glDeleteTextures(1, &glTexture);
        }
    }

    // 2. CPU tarafındaki tüm haritaları (map) boşaltıyoruz
    textures.clear();
    texturePaths.clear();

    // 3. ID sayacını sıfırlıyoruz
    nextID = 0;

    std::cout << "[TextureManager] VRAM'deki tüm dokular (textures) silindi ve sıfırlandı." << std::endl;
}