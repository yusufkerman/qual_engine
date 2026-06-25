#include "grid_renderer.h"

static float quadVertices[] =
{
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,

    -1.0f,  1.0f,
     1.0f, -1.0f,
     1.0f,  1.0f
};

void GridRenderer::init()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
}

void GridRenderer::draw()
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void GridRenderer::shutdown()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}