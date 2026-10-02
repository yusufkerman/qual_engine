/**
 * @file shader_manager.cpp
 * @brief [LEGACY] Eski shader yöneticisi uygulaması.
 */

#include "shader_manager.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) 
#endif

unsigned int shader_manager::load(const char* vs, const char* fs)
{
    const unsigned int id = nextID++;

    // Yeni shader oluştur ve sözlüğe (map) ekle
    shaders.emplace(id, shader(vs, fs));

    return id;
}

shader& shader_manager::get(unsigned int id)
{
    return shaders.at(id);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif