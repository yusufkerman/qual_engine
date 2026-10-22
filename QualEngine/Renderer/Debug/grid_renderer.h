#pragma once
#include <glad/glad.h>

class GridRenderer
{
public:
    void init();
    void draw();   // artık parametre yok
    void shutdown();

private:
    unsigned int vao = 0;
    unsigned int vbo = 0;
};