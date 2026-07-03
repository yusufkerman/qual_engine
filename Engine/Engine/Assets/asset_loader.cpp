#include "asset_loader.h"
#include "../Externals/stb/stb_image.h"
#include "../Externals/nlohmann/json.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <limits>

using json = nlohmann::json;

namespace aria {

    // Helper: Assimp köşe verisini bizim raw_vertex yapımıza dönüştürür
    static raw_vertex process_assimp_vertex(aiMesh* mesh, unsigned int i) {
        raw_vertex v;

        v.position.x = mesh->mVertices[i].x;
        v.position.y = mesh->mVertices[i].y;
        v.position.z = mesh->mVertices[i].z;

        if (mesh->HasNormals()) {
            v.normal.x = mesh->mNormals[i].x;
            v.normal.y = mesh->mNormals[i].y;
            v.normal.z = mesh->mNormals[i].z;
        }

        if (mesh->mTextureCoords[0]) {
            v.tex_coord.x = mesh->mTextureCoords[0][i].x;
            v.tex_coord.y = mesh->mTextureCoords[0][i].y;
        }

        if (mesh->HasTangentsAndBitangents()) {
            v.tangent.x = mesh->mTangents[i].x;
            v.tangent.y = mesh->mTangents[i].y;
            v.tangent.z = mesh->mTangents[i].z;
        }

        return v;
    }

    // Helper: Orijinal ComputeBounds mantığın
    static void compute_mesh_bounds(raw_mesh_data& out_mesh) {
        if (out_mesh.vertices.empty()) {
            out_mesh.local_center = glm::vec3(0.0f);
            out_mesh.local_radius = 0.0f;
            return;
        }

        glm::vec3 min_b(std::numeric_limits<float>::max());
        glm::vec3 max_b(std::numeric_limits<float>::lowest());

        for (const auto& v : out_mesh.vertices) {
            min_b = glm::min(min_b, v.position);
            max_b = glm::max(max_b, v.position);
        }

        out_mesh.local_center = (min_b + max_b) * 0.5f;

        float max_dist_sq = 0.0f;
        for (const auto& v : out_mesh.vertices) {
            glm::vec3 diff = v.position - out_mesh.local_center;
            max_dist_sq = glm::max(max_dist_sq, glm::dot(diff, diff));
        }

        out_mesh.local_radius = glm::sqrt(max_dist_sq);
    }

    // Helper: Tekil bir Assimp mesh'ini işler
    static raw_mesh_data process_assimp_mesh(aiMesh* mesh) {
        raw_mesh_data out_mesh;

        out_mesh.vertices.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            out_mesh.vertices.push_back(process_assimp_vertex(mesh, i));
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                out_mesh.indices.push_back(face.mIndices[j]);
            }
        }

        compute_mesh_bounds(out_mesh);
        return out_mesh;
    }

    // Helper: Assimp düğüm ağacını recursive olarak tarar
    static void process_assimp_node(aiNode* node, const aiScene* scene, raw_model_data& out_model) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            out_model.sub_meshes.push_back(process_assimp_mesh(mesh));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            process_assimp_node(node->mChildren[i], scene, out_model);
        }
    }

    std::optional<raw_texture_data> asset_loader::load_texture_to_cpu(const std::string& file_path) {
        if (file_path.empty()) {
            std::cout << "[ARIA Loader] Error: Texture file path is empty.\n";
            return std::nullopt;
        }

        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            std::cout << "[ARIA Loader] Error: Failed to load texture file: " << file_path << "\n";
            return std::nullopt;
        }

        raw_texture_data out_raw_data;
        out_raw_data.debug_path = file_path;
        out_raw_data.width = width;
        out_raw_data.height = height;
        out_raw_data.channels = channels;

        size_t total_size = static_cast<size_t>(width) * height * channels;
        out_raw_data.pixels.resize(total_size);
        std::memcpy(out_raw_data.pixels.data(), data, total_size);

        stbi_image_free(data);
        return out_raw_data;
    }

    std::optional<raw_model_data> asset_loader::load_model_to_cpu(const std::string& file_path) {
        if (file_path.empty()) {
            std::cout << "[ARIA Loader] Error: Model file path is empty.\n";
            return std::nullopt;
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            file_path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_CalcTangentSpace
        );

        if (!scene || !scene->mRootNode) {
            std::cout << "[ARIA Loader ERROR] Assimp failed for file: " << file_path << "\n";
            std::cout << "[Assimp Error String] " << importer.GetErrorString() << std::endl;
            return std::nullopt;
        }

        raw_model_data out_model_data;
        out_model_data.debug_path = file_path;

        process_assimp_node(scene->mRootNode, scene, out_model_data);

        return out_model_data;
    }

    std::optional<raw_material_data> asset_loader::load_material_to_cpu(const std::string& file_path) {
        if (file_path.empty()) {
            std::cout << "[ARIA Loader] Error: Material file path is empty.\n";
            return std::nullopt;
        }

        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cout << "[ARIA Loader] Error: Failed to open material file: " << file_path << "\n";
            return std::nullopt;
        }

        json material_json;
        try {
            file >> material_json;
        }
        catch (const json::parse_error& e) {
            std::cout << "[ARIA Loader ERROR] JSON Parse failed: " << e.what() << "\n";
            file.close();
            return std::nullopt;
        }
        file.close();

        try {
            raw_material_data out_raw_data;
            out_raw_data.debug_path = file_path;
            out_raw_data.name = material_json.value("name", "Unnamed_Material");

            // 🎯 RENGİ GÜVENLİ OKUMA (Aynen korundu)
            if (material_json.contains("base_color")) {
                auto& bc = material_json["base_color"];
                if (bc.is_array()) {
                    out_raw_data.base_color.x = bc.size() > 0 ? bc[0].get<float>() : 1.0f;
                    out_raw_data.base_color.y = bc.size() > 1 ? bc[1].get<float>() : 1.0f;
                    out_raw_data.base_color.z = bc.size() > 2 ? bc[2].get<float>() : 1.0f;
                }
                else if (bc.is_object()) {
                    out_raw_data.base_color.x = bc.value("x", 1.0f);
                    out_raw_data.base_color.y = bc.value("y", 1.0f);
                    out_raw_data.base_color.z = bc.value("z", 1.0f);
                }
            }
            else {
                out_raw_data.base_color.x = 1.0f;
                out_raw_data.base_color.y = 1.0f;
                out_raw_data.base_color.z = 1.0f;
            }

            // Standart Parametreler
            out_raw_data.specular_strength = material_json.value("specular_strength", 0.5f);
            out_raw_data.shininess = material_json.value("shininess", 32.0f);
            out_raw_data.normal_strength = material_json.value("normal_strength", 1.0f);

            // Texture Scale Kontrolü
            if (material_json.contains("texture_scale")) {
                auto& ts = material_json["texture_scale"];
                if (ts.is_object()) {
                    out_raw_data.texture_scale.x = ts.value("x", 1.0f);
                    out_raw_data.texture_scale.y = ts.value("y", 1.0f);
                }
                else if (ts.is_array()) {
                    out_raw_data.texture_scale.x = ts.size() > 0 ? ts[0].get<float>() : 1.0f;
                    out_raw_data.texture_scale.y = ts.size() > 1 ? ts[1].get<float>() : 1.0f;
                }
            }
            else {
                out_raw_data.texture_scale.x = 1.0f;
                out_raw_data.texture_scale.y = 1.0f;
            }

            // Helper Lambda: JSON'dan string okuyup güvenli şekilde aria::asset_guid yapısına çevirir
            auto parse_guid_helper = [](const json& j, const std::string& key) -> aria::asset_guid {
                std::string guid_str = j.value(key, "");
                if (guid_str.empty()) return {}; // Invalid/Empty GUID döner

                auto parsed = uuids::uuid::from_string(guid_str);
                return parsed ? *parsed : aria::asset_guid{};
                };

            // 🎯 ARTIK PATH DEĞİL, GEÇERLİ GUID'LERİ SİSTEME DOLDURUYORUZ
            out_raw_data.shader_guid = parse_guid_helper(material_json, "shader_guid");
            out_raw_data.diffuse_texture_guid = parse_guid_helper(material_json, "diffuse_texture_guid");
            out_raw_data.normal_texture_guid = parse_guid_helper(material_json, "normal_texture_guid");
            out_raw_data.specular_texture_guid = parse_guid_helper(material_json, "specular_texture_guid");

            return out_raw_data;
        }
        catch (const nlohmann::json::exception& e) {
            std::cout << "[ARIA Loader ERROR] Runtime JSON Type Mismatch: " << e.what() << "\n";
            return std::nullopt;
        }
    }
    std::optional<raw_shader_data> asset_loader::load_shader_to_cpu(const std::string& file_path) {
        std::cout << "[Aria Loader] Loading shader from path: " << file_path << "\n";

        // .vert ve .frag dosyalarını oku
        std::string vert_path = file_path + ".vert";
        std::string frag_path = file_path + ".frag";

        std::cout << "[Aria Loader] Vertex path: " << vert_path << "\n";
        std::cout << "[Aria Loader] Fragment path: " << frag_path << "\n";

        std::ifstream vFile(vert_path);
        std::ifstream fFile(frag_path);

        if (!vFile.is_open()) {
            std::cout << "[Aria Loader] ERROR: Vertex shader file not found: " << vert_path << std::endl;
        }
        if (!fFile.is_open()) {
            std::cout << "[Aria Loader] ERROR: Fragment shader file not found: " << frag_path << std::endl;
        }

        if (!vFile.is_open() || !fFile.is_open()) {
            std::cout << "[Aria Loader] Shader files not found: " << file_path << std::endl;
            return std::nullopt;
        }

        raw_shader_data raw;
        raw.debug_path = file_path;

        std::stringstream vStr, fStr;
        vStr << vFile.rdbuf();
        fStr << fFile.rdbuf();

        raw.vertex_source = vStr.str();
        raw.fragment_source = fStr.str();

        vFile.close();
        fFile.close();

        return raw;
    }

} // namespace aria