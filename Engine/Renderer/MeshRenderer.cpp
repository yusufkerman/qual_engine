#include "MeshRenderer.h"
#include <glad/glad.h>
#include <iostream>

struct GPU_Mesh
{
    unsigned int VAO, VBO, EBO;
    int indexCount;
};

static std::vector<GPU_Mesh> gpuMeshes;
static int nextMeshID = 0;

static GPU_Mesh& GetMesh(int meshID)
{
    return gpuMeshes[meshID];
}

int MeshRenderer::UploadModel(const Model& model)
{
    int baseID = nextMeshID;

    for (const auto& mesh : model.meshes)
    {
        GPU_Mesh g;

        glGenVertexArrays(1, &g.VAO);
        glGenBuffers(1, &g.VBO);
        glGenBuffers(1, &g.EBO);

        glBindVertexArray(g.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, g.VBO);
        glBufferData(GL_ARRAY_BUFFER,
            mesh.vertices.size() * sizeof(Vertex),
            mesh.vertices.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            mesh.indices.size() * sizeof(unsigned int),
            mesh.indices.data(),
            GL_STATIC_DRAW);


        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);


        g.indexCount = (int)mesh.indices.size();

        gpuMeshes.push_back(g);
        nextMeshID++;
    }

    return baseID;
}

void MeshRenderer::DrawModel()
{
    for (auto& m : gpuMeshes)
    {
        glBindVertexArray(m.VAO);
        glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
    }
}

void MeshRenderer::Draw(const render_item& item, texture_manager& textureManager)
{
    GPU_Mesh& m = GetMesh(item.mesh_id);
    glBindVertexArray(m.VAO);

    // ------------------------------------------------------------------------
    // TEXTURE BINDING (DOKULARI BAĞLAMA)
    // ------------------------------------------------------------------------
    if (item.diffuse_texture != 0)
    {
        unsigned int realGLid = textureManager.get(item.diffuse_texture);
        glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, realGLid);
    }

    if (item.normal_texture != 0)
    {
        unsigned int realNormalGLid = textureManager.get(item.normal_texture);
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, realNormalGLid);
    }

    // ------------------------------------------------------------------------
    // ACTUAL DRAW CALL (GERÇEK ÇİZİM)
    // ------------------------------------------------------------------------
    glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);

    // ------------------------------------------------------------------------
    //  STATE LEAK PROTECTION (UNBIND KORUMASI)
    // ------------------------------------------------------------------------
    if (item.diffuse_texture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (item.normal_texture != 0)
    {
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glBindVertexArray(0);
}

void MeshRenderer::Clear()
{
    for (auto& m : gpuMeshes)
    {
        glDeleteVertexArrays(1, &m.VAO);
        glDeleteBuffers(1, &m.VBO);
        glDeleteBuffers(1, &m.EBO);
    }

    gpuMeshes.clear();
    nextMeshID = 0;

    std::cout << "[MeshRenderer] VRAM temizlendi, tüm meshler silindi." << std::endl;
}

// 🛠️ YENİ EKLEMELER: Gelen state isteklerini karşılayan gövdeler
void MeshRenderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void MeshRenderer::BindFramebuffer(unsigned int fboID)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
}

void MeshRenderer::BindTextureSlot(unsigned int slot, unsigned int textureID)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
}