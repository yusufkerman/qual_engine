#pragma once
#include "shader.h"

class shader_manager
{
public:
    unsigned int load(const char* vs, const char* fs);
    shader& get(unsigned int id);

private:
    std::unordered_map<unsigned int, shader> shaders;
    unsigned int nextID = 0;
};