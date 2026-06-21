#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>

#include <iostream>
#include <limits>

// -------------------------
// Vertex
// -------------------------
static Vertex ProcessVertex(aiMesh* mesh, unsigned int i)
{
    Vertex v;

    v.Position[0] = mesh->mVertices[i].x;
    v.Position[1] = mesh->mVertices[i].y;
    v.Position[2] = mesh->mVertices[i].z;

    if (mesh->HasNormals())
    {
        v.Normal[0] = mesh->mNormals[i].x;
        v.Normal[1] = mesh->mNormals[i].y;
        v.Normal[2] = mesh->mNormals[i].z;
    }
    else
    {
        v.Normal[0] = v.Normal[1] = v.Normal[2] = 0.0f;
    }

    if (mesh->mTextureCoords[0])
    {
        v.TexCoord[0] = mesh->mTextureCoords[0][i].x;
        v.TexCoord[1] = mesh->mTextureCoords[0][i].y;
    }
    else
    {
        v.TexCoord[0] = 0.0f;
        v.TexCoord[1] = 0.0f;
    }


    if (mesh->HasTangentsAndBitangents())
    {
        v.Tangent[0] = mesh->mTangents[i].x;
        v.Tangent[1] = mesh->mTangents[i].y;
        v.Tangent[2] = mesh->mTangents[i].z;
    }
    else
    {
        v.Tangent[0] = v.Tangent[1] = v.Tangent[2] = 0.0f;
    }

    return v;
}

// -------------------------
// Bounds (YENİ)
// -------------------------
static void ComputeBounds(Mesh& out)
{
    if (out.vertices.empty())
    {
        out.localCenter = glm::vec3(0.0f);
        out.localRadius = 0.0f;
        return;
    }

    glm::vec3 minB(std::numeric_limits<float>::max());
    glm::vec3 maxB(std::numeric_limits<float>::lowest());

    for (const auto& v : out.vertices)
    {
        glm::vec3 p(v.Position[0], v.Position[1], v.Position[2]);
        minB = glm::min(minB, p);
        maxB = glm::max(maxB, p);
    }

    out.localCenter = (minB + maxB) * 0.5f;

    float maxDistSq = 0.0f;
    for (const auto& v : out.vertices)
    {
        glm::vec3 p(v.Position[0], v.Position[1], v.Position[2]);
        glm::vec3 diff = p - out.localCenter;
        maxDistSq = glm::max(maxDistSq, glm::dot(diff, diff));
    }

    out.localRadius = glm::sqrt(maxDistSq);
}

// -------------------------
// Mesh
// -------------------------
static Mesh ProcessMesh(aiMesh* mesh)
{
    Mesh out;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        out.vertices.push_back(ProcessVertex(mesh, i));

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
            out.indices.push_back(face.mIndices[j]);
    }

    ComputeBounds(out); // YENİ

    return out;
}

// -------------------------
// Node recursive
// -------------------------
static void ProcessNode(aiNode* node, const aiScene* scene, Model& model)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(ProcessMesh(mesh));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        ProcessNode(node->mChildren[i], scene, model);
}

// -------------------------
// LOAD
// -------------------------
Model ModelLoader::Load(const std::string& path)
{
    Assimp::Importer importer;

    std::cout << "[ModelLoader] Loading: " << path << std::endl;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace
    );

    Model model;

    // -------------------------
    // ERROR HANDLING
    // -------------------------
    if (!scene)
    {
        std::cout << "[ModelLoader ERROR] Scene is NULL!\n";
        std::cout << "[Assimp Error] " << importer.GetErrorString() << std::endl;
        return model;
    }

    if (!scene->mRootNode)
    {
        std::cout << "[ModelLoader ERROR] RootNode is NULL!\n";
        std::cout << "[Assimp Error] " << importer.GetErrorString() << std::endl;
        return model;
    }

    std::cout << "[ModelLoader] Mesh count: " << scene->mNumMeshes << std::endl;

    ProcessNode(scene->mRootNode, scene, model);

    std::cout << "[ModelLoader] Loaded successfully!\n";

    return model;
}
