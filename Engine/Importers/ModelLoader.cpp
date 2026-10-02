/**
 * @file ModelLoader.cpp
 * @brief Assimp tabanlı model yükleme, tepe noktası çözümleme ve sınır hacmi hesaplama uygulaması.
 */

#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>

namespace {

    /**
     * @brief Tekil bir Assimp tepe noktasını motorun Vertex yapısına dönüştürür.
     */
    Vertex ProcessVertex(const aiMesh* mesh, unsigned int i)
    {
        Vertex v;

        v.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if (mesh->HasNormals())
        {
            v.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        else
        {
            v.Normal = glm::vec3(0.0f);
        }

        if (mesh->mTextureCoords[0])
        {
            v.TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else
        {
            v.TexCoord = glm::vec2(0.0f);
        }

        if (mesh->HasTangentsAndBitangents())
        {
            v.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        }
        else
        {
            v.Tangent = glm::vec3(0.0f);
        }

        return v;
    }

    /**
     * @brief Ağın yerel merkezini ve çevreleyen küre yarıçapını (Bounding Sphere) hesaplar.
     */
    void ComputeBounds(Mesh& out)
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
            minB = glm::min(minB, v.Position);
            maxB = glm::max(maxB, v.Position);
        }

        out.localCenter = (minB + maxB) * 0.5f;

        float maxDistSq = 0.0f;
        for (const auto& v : out.vertices)
        {
            glm::vec3 diff = v.Position - out.localCenter;
            maxDistSq = glm::max(maxDistSq, glm::dot(diff, diff));
        }

        out.localRadius = glm::sqrt(maxDistSq);
    }

    /**
     * @brief Assimp aiMesh nesnesini motorun Mesh yapısına kopyalar.
     */
    Mesh ProcessMesh(const aiMesh* mesh)
    {
        Mesh out;
        out.name = mesh->mName.C_Str();

        out.vertices.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            out.vertices.push_back(ProcessVertex(mesh, i));
        }

        out.indices.reserve(mesh->mNumFaces * 3);
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                out.indices.push_back(face.mIndices[j]);
            }
        }

        ComputeBounds(out);
        return out;
    }

    /**
     * @brief Assimp sahne hiyerarşisini özyinelemeli (recursive) olarak dolaşarak modelleri toplar.
     */
    void ProcessNode(const aiNode* node, const aiScene* scene, Model& model)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            model.meshes.push_back(ProcessMesh(mesh));
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            ProcessNode(node->mChildren[i], scene, model);
        }
    }

} // anonymous namespace

Model ModelLoader::Load(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace
    );

    Model model;

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        std::cerr << "[ModelLoader] Model yuklenemedi: " << path << '\n'
            << "[Assimp Hatasi] " << importer.GetErrorString() << '\n';
        return model;
    }

    ProcessNode(scene->mRootNode, scene, model);
    return model;
}