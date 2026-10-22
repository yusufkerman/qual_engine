#pragma once

#include <glad/glad.h>

class RenderTarget
{
public:
    unsigned int fbo = 0;
    unsigned int tex = 0;
    unsigned int rbo = 0;

    int width = 1;
    int height = 1;

    void create(int w, int h);
    void resize(int w, int h);
    void destroy();
};