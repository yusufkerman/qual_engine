#include "texture_manager.h"
#include <glad/glad.h>
#include <iostream>
#include "../Importers/texture_loader.h"



texture_manager::texture_manager()
    : m_FallbackTextureGLID(0) // Constructor sadece güvenli ilk atamayı yapar
{
}

void texture_manager::init()
{
    // Çünkü bu fonksiyonu gladLoadGL çağrısından SONRA tetikleyeceğiz.
    if (m_FallbackTextureGLID != 0) return; // Zaten kurulduysa pas geç

    glGenTextures(1, &m_FallbackTextureGLID);
    glBindTexture(GL_TEXTURE_2D, m_FallbackTextureGLID);

    unsigned char whitePixel[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "[TextureManager] Fallback texture successfully created." << std::endl;
}
texture_manager::~texture_manager()
{
    if (m_FallbackTextureGLID != 0)
    {
        glDeleteTextures(1, &m_FallbackTextureGLID);
    }
}

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
    texturePaths[id] = path; //Yüklenen dokunun dosya yolunu kaydettik!

    return id;
}

unsigned int texture_manager::get(unsigned int id)
{
    auto it = textures.find(id);
    if (it == textures.end())
    {
        // Geçersiz veya henüz yüklenmemiş bir ID istendiğinde motor çökmez, 
        // ekrana morluk basmak yerine düz beyaz fallback dokusunu döner.
        return m_FallbackTextureGLID;
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