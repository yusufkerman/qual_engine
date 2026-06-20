#include "shader_manager.h"


unsigned int shader_manager::load(const char* vs, const char* fs)
{
    unsigned int id = nextID++;

    shaders.emplace(id, shader(vs, fs));

    return id;
}
shader& shader_manager::get(unsigned int id)
{
    return shaders.at(id);
}