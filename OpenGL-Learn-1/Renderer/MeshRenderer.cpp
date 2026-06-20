#include "MeshRenderer.h"
#include <glad/glad.h>

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
//void MeshRenderer::DrawMesh(int meshID)
//{
//    GPU_Mesh& m = GetMesh(meshID);
//
//    glBindVertexArray(m.VAO);
//    glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
//}
void MeshRenderer::Draw(const render_item& item)
{
    GPU_Mesh& m = GetMesh(item.mesh_id);

    glBindVertexArray(m.VAO);

    // şimdilik sadece model matrix
    // shader setleme kısmı ileride material'a taşınacak

    glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
}