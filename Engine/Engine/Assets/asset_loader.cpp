/**
 * @file asset_loader.cpp
 * @brief Varlık (asset) dosyalarını diskten okuyup CPU tarafındaki ham veri yapılarına dönüştüren fonksiyonları içerir.
 */

#include "asset_loader.h"
#include "../../Core/path_utils.h"
#include "../Externals/miniaudio.h"
#include "../Externals/stb/stb_image.h"
#include "../Externals/nlohmann/json.hpp"
#include "../../Resources/engine_resources.h"
#include "../../Core/VFS/vfs.h"
#include "aria.h"
#include "../asset_pipeline.h"
#include "../../Gameplay/gameplay_context.h"
#include "../engine_context.h"
#include "../../Project/project.h"
#include <filesystem>
#include <cmath>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <limits>

using json = nlohmann::json;

namespace aria {

    static glm::mat4 assimp_to_glm_mat4(const aiMatrix4x4& from) {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    static glm::vec3 assimp_to_glm_vec3(const aiVector3D& vec) {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    static glm::quat assimp_to_glm_quat(const aiQuaternion& quat) {
        return glm::quat(quat.w, quat.x, quat.y, quat.z);
    }

    /**
     * @brief Assimp köşe (vertex) verisini motorun kendi raw_vertex yapısına dönüştürür.
     *
     * @param mesh İşlenen Assimp mesh nesnesi.
     * @param i İşlenecek köşenin indeksi.
     * @return raw_vertex Motorun kullanabileceği formattaki köşe verisi.
     */
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

    /**
     * @brief Verilen ham mesh verisi için yerel sınırlayıcı kutu (bounding box) ve yarıçap (radius) hesaplar.
     *
     * @param out_mesh Sınırları hesaplanacak ve güncellenecek olan mesh nesnesi.
     */
    static void compute_mesh_bounds(raw_mesh_data& out_mesh) {
        if (out_mesh.vertices.empty()) {
            out_mesh.local_center = glm::vec3(0.0f);
            out_mesh.local_radius = 0.0f;
            out_mesh.aabb_min = glm::vec3(0.0f);
            out_mesh.aabb_max = glm::vec3(0.0f);
            return;
        }

        glm::vec3 min_b(std::numeric_limits<float>::max());
        glm::vec3 max_b(std::numeric_limits<float>::lowest());

        for (const auto& v : out_mesh.vertices) {
            min_b = glm::min(min_b, v.position);
            max_b = glm::max(max_b, v.position);
        }

        out_mesh.local_center = (min_b + max_b) * 0.5f;
        out_mesh.aabb_min = min_b;
        out_mesh.aabb_max = max_b;

        float max_dist_sq = 0.0f;
        for (const auto& v : out_mesh.vertices) {
            glm::vec3 diff = v.position - out_mesh.local_center;
            max_dist_sq = glm::max(max_dist_sq, glm::dot(diff, diff));
        }

        out_mesh.local_radius = glm::sqrt(max_dist_sq);
    }

    /**
     * @brief Assimp tarafından yüklenen bir mesh parçasını motorun anlayacağı yapıya ayrıştırır.
     *
     * @param mesh Ayrıştırılacak Assimp mesh nesnesi.
     * @return raw_mesh_data Ayrıştırılmış, CPU tarafında tutulacak ham mesh verisi.
     */
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

    /**
     * @brief Assimp sahne ağacındaki (scene graph) düğümleri özyinelemeli (recursive) olarak dolaşır.
     *
     * @param node Üzerinde işlem yapılan mevcut Assimp düğümü.
     * @param scene Düğümlerin bağlı olduğu ana Assimp sahnesi.
     * @param out_model Çıkartılan mesh verilerinin ekleneceği model nesnesi.
     */
    static void process_assimp_node(aiNode* node, const aiScene* scene, raw_model_data& out_model, const glm::mat4& parent_transform = glm::mat4(1.0f)) {
        const glm::mat4 node_local = assimp_to_glm_mat4(node->mTransformation);
        const glm::mat4 current_transform = parent_transform * node_local;

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            raw_mesh_data r_mesh = process_assimp_mesh(mesh);
            
            // Sub-mesh adı: Eğer mesh'in adı geçerliyse onu kullan, yoksa node adını kullan
            if (mesh->mName.length > 0) {
                r_mesh.name = mesh->mName.C_Str();
            } else if (node->mName.length > 0) {
                r_mesh.name = node->mName.C_Str();
            } else {
                r_mesh.name = "SubMesh_" + std::to_string(out_model.sub_meshes.size());
            }

            r_mesh.local_transform = current_transform;
            out_model.sub_meshes.push_back(std::move(r_mesh));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            process_assimp_node(node->mChildren[i], scene, out_model, current_transform);
        }
    }

    /**
     * @brief İskeletli ham mesh verisi için yerel sınırlayıcı kutu (bounding box) ve yarıçap (radius) hesaplar.
     *
     * @param out_mesh Sınırları hesaplanacak ve güncellenecek olan derili mesh nesnesi.
     */
    static void compute_skinned_mesh_bounds(raw_skinned_mesh_data& out_mesh) {
        if (out_mesh.vertices.empty()) {
            out_mesh.local_center = glm::vec3(0.0f);
            out_mesh.local_radius = 0.0f;
            out_mesh.aabb_min = glm::vec3(0.0f);
            out_mesh.aabb_max = glm::vec3(0.0f);
            return;
        }

        glm::vec3 min_b(std::numeric_limits<float>::max());
        glm::vec3 max_b(std::numeric_limits<float>::lowest());

        for (const auto& v : out_mesh.vertices) {
            min_b = glm::min(min_b, v.position);
            max_b = glm::max(max_b, v.position);
        }

        out_mesh.local_center = (min_b + max_b) * 0.5f;
        out_mesh.aabb_min = min_b;
        out_mesh.aabb_max = max_b;

        float max_dist_sq = 0.0f;
        for (const auto& v : out_mesh.vertices) {
            glm::vec3 diff = v.position - out_mesh.local_center;
            max_dist_sq = glm::max(max_dist_sq, glm::dot(diff, diff));
        }

        out_mesh.local_radius = glm::sqrt(max_dist_sq);
    }

    /**
     * @brief Assimp tarafından yüklenen kemikli/derili bir mesh parçasını ve kemik ağırlıklarını ayrıştırır.
     */
    static raw_skinned_mesh_data process_assimp_skinned_mesh(aiMesh* mesh, raw_skeleton_data& skeleton) {
        raw_skinned_mesh_data out_mesh;
        out_mesh.vertices.resize(mesh->mNumVertices);

        // 1. Temel köşe özelliklerini kopyala
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            raw_skinned_vertex& v = out_mesh.vertices[i];
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

            v.bone_ids = glm::ivec4(-1);
            v.bone_weights = glm::vec4(0.0f);
        }

        // 2. Kemikleri ve köşe ağırlıklarını işle
        for (unsigned int b = 0; b < mesh->mNumBones; b++) {
            aiBone* bone = mesh->mBones[b];
            std::string bone_name = bone->mName.C_Str();

            int bone_id = -1;
            auto it = skeleton.bone_mapping.find(bone_name);
            if (it == skeleton.bone_mapping.end()) {
                bone_id = static_cast<int>(skeleton.bones.size());
                bone_info new_bone;
                new_bone.id = bone_id;
                new_bone.name = bone_name;
                new_bone.offset_matrix = assimp_to_glm_mat4(bone->mOffsetMatrix);

                skeleton.bones.push_back(new_bone);
                skeleton.bone_mapping[bone_name] = bone_id;
            }
            else {
                bone_id = it->second;
            }

            for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                unsigned int vertex_id = bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;

                if (vertex_id >= out_mesh.vertices.size()) continue;

                raw_skinned_vertex& v = out_mesh.vertices[vertex_id];

                // Boş yuva bul ve yerleştir
                bool added = false;
                for (int k = 0; k < MAX_BONES_PER_VERTEX; k++) {
                    if (v.bone_ids[k] < 0 || v.bone_weights[k] == 0.0f) {
                        v.bone_ids[k] = bone_id;
                        v.bone_weights[k] = weight;
                        added = true;
                        break;
                    }
                }

                // Eğer 4 yuva da doluysa ve yeni ağırlık mevcutlardan birinden büyükse, en küçüğü değiştir
                if (!added) {
                    int min_idx = 0;
                    float min_w = v.bone_weights[0];
                    for (int k = 1; k < MAX_BONES_PER_VERTEX; k++) {
                        if (v.bone_weights[k] < min_w) {
                            min_w = v.bone_weights[k];
                            min_idx = k;
                        }
                    }
                    if (weight > min_w) {
                        v.bone_ids[min_idx] = bone_id;
                        v.bone_weights[min_idx] = weight;
                    }
                }
            }
        }

        // 3. Ağırlıkları normalize et ve negatif indeksleri sıfırla
        for (auto& v : out_mesh.vertices) {
            float total_w = v.bone_weights.x + v.bone_weights.y + v.bone_weights.z + v.bone_weights.w;
            if (total_w > 0.00001f) {
                v.bone_weights /= total_w;
                for (int k = 0; k < MAX_BONES_PER_VERTEX; k++) {
                    if (v.bone_ids[k] < 0) {
                        v.bone_ids[k] = 0;
                        v.bone_weights[k] = 0.0f;
                    }
                }
            }
            else {
                v.bone_ids = glm::ivec4(0);
                v.bone_weights = glm::vec4(0.0f);
            }
        }

        // 4. İndeksleri kopyala
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                out_mesh.indices.push_back(face.mIndices[j]);
            }
        }

        // 5. Yerel sınırları hesapla
        compute_skinned_mesh_bounds(out_mesh);

        return out_mesh;
    }

    /**
     * @brief Assimp sahne ağacından iskelet hiyerarşisi düğümlerini özyinelemeli inşa eder.
     */
    static skeleton_node build_skeleton_node(const aiNode* node, const std::unordered_map<std::string, int>& bone_map) {
        skeleton_node sk_node;
        sk_node.name = node->mName.C_Str();
        sk_node.local_transform = assimp_to_glm_mat4(node->mTransformation);

        auto it = bone_map.find(sk_node.name);
        if (it != bone_map.end()) {
            sk_node.bone_index = it->second;
        }
        else {
            sk_node.bone_index = -1;
        }

        sk_node.children.reserve(node->mNumChildren);
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            sk_node.children.push_back(build_skeleton_node(node->mChildren[i], bone_map));
        }

        return sk_node;
    }

    /**
     * @brief Assimp animasyon klibini motorun ham animasyon verisi yapısına dönüştürür.
     */
    static raw_animation_data process_assimp_animation(const aiAnimation* anim) {
        raw_animation_data anim_data;
        anim_data.name = anim->mName.C_Str();
        anim_data.duration = static_cast<float>(anim->mDuration);
        anim_data.ticks_per_second = anim->mTicksPerSecond != 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;

        anim_data.channels.reserve(anim->mNumChannels);
        for (unsigned int i = 0; i < anim->mNumChannels; i++) {
            aiNodeAnim* channel = anim->mChannels[i];
            bone_animation_channel bone_channel;
            bone_channel.bone_name = channel->mNodeName.C_Str();

            bone_channel.positions.reserve(channel->mNumPositionKeys);
            for (unsigned int p = 0; p < channel->mNumPositionKeys; p++) {
                bone_channel.positions.push_back({
                    static_cast<float>(channel->mPositionKeys[p].mTime),
                    assimp_to_glm_vec3(channel->mPositionKeys[p].mValue)
                });
            }

            bone_channel.rotations.reserve(channel->mNumRotationKeys);
            for (unsigned int r = 0; r < channel->mNumRotationKeys; r++) {
                bone_channel.rotations.push_back({
                    static_cast<float>(channel->mRotationKeys[r].mTime),
                    assimp_to_glm_quat(channel->mRotationKeys[r].mValue)
                });
            }

            bone_channel.scalings.reserve(channel->mNumScalingKeys);
            for (unsigned int s = 0; s < channel->mNumScalingKeys; s++) {
                bone_channel.scalings.push_back({
                    static_cast<float>(channel->mScalingKeys[s].mTime),
                    assimp_to_glm_vec3(channel->mScalingKeys[s].mValue)
                });
            }

            anim_data.channel_mapping[bone_channel.bone_name] = anim_data.channels.size();
            anim_data.channels.push_back(std::move(bone_channel));
        }

        return anim_data;
    }

    /**
     * @brief Assimp sahne ağacındaki düğümleri özyinelemeli dolaşarak derili mesh'leri ayrıştırır.
     */
    static void process_assimp_skinned_node(aiNode* node, const aiScene* scene, raw_skinned_model_data& out_model) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            out_model.sub_meshes.push_back(process_assimp_skinned_mesh(mesh, out_model.skeleton));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            process_assimp_skinned_node(node->mChildren[i], scene, out_model);
        }
    }

    std::optional<raw_texture_data> asset_loader::load_texture_to_cpu(const std::string& file_path) {
        if (file_path.empty()) {
            std::cout << "[ARIA Loader] Hata: Doku dosya yolu bos.\n";
            return std::nullopt;
        }

        const std::string resolved_path = engine_resources::resolve_path(file_path);

        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = nullptr;
        std::vector<uint8_t> vfs_tex_bytes;

        if (qual::vfs::instance().has_file(file_path)) {
            vfs_tex_bytes = qual::vfs::instance().read_bytes(file_path);
            if (!vfs_tex_bytes.empty()) {
                data = stbi_load_from_memory(vfs_tex_bytes.data(), static_cast<int>(vfs_tex_bytes.size()), &width, &height, &channels, 0);
            }
        }
        if (!data) {
            std::ifstream disk_file(qual::path_from_utf8(resolved_path), std::ios::binary | std::ios::ate);
            if (disk_file.is_open()) {
                const std::streamsize file_size = disk_file.tellg();
                if (file_size > 0) {
                    disk_file.seekg(0, std::ios::beg);
                    std::vector<uint8_t> file_bytes(static_cast<size_t>(file_size));
                    if (disk_file.read(reinterpret_cast<char*>(file_bytes.data()), file_size)) {
                        data = stbi_load_from_memory(file_bytes.data(), static_cast<int>(file_bytes.size()), &width, &height, &channels, 0);
                    }
                }
            }
        }
        if (!data) {
            data = stbi_load(resolved_path.c_str(), &width, &height, &channels, 0);
        }

        if (!data) {
            std::cout << "[ARIA Loader] Hata: Doku dosyasi yuklenemedi: " << resolved_path << "\n";
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
            std::cout << "[ARIA Loader] Hata: Model dosya yolu bos.\n";
            return std::nullopt;
        }

        Assimp::Importer importer;
        constexpr unsigned int model_pFlags = aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;
        const aiScene* scene = nullptr;
        std::vector<uint8_t> vfs_model_bytes;

        if (qual::vfs::instance().has_file(file_path)) {
            vfs_model_bytes = qual::vfs::instance().read_bytes(file_path);
            if (!vfs_model_bytes.empty()) {
                std::string ext = std::filesystem::path(file_path).extension().string();
                if (!ext.empty() && ext.front() == '.') ext.erase(0, 1);
                scene = importer.ReadFileFromMemory(vfs_model_bytes.data(), vfs_model_bytes.size(), model_pFlags, ext.c_str());
            }
        }
        if (!scene) {
            scene = importer.ReadFile(file_path, model_pFlags);
        }

        if (!scene || !scene->mRootNode) {
            std::cout << "[ARIA Loader] Hata: Assimp yukleme basarisiz oldu: " << file_path << "\n";
            std::cout << "[Assimp Hata Detayi] " << importer.GetErrorString() << std::endl;
            return std::nullopt;
        }

        raw_model_data out_model_data;
        out_model_data.debug_path = file_path;

        process_assimp_node(scene->mRootNode, scene, out_model_data);

        return out_model_data;
    }

    std::optional<raw_skinned_model_data> asset_loader::load_skinned_model_to_cpu(const std::string& file_path) {
        if (file_path.empty()) {
            std::cout << "[ARIA Loader] Hata: Skinned model dosya yolu bos.\n";
            return std::nullopt;
        }

        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
        const aiScene* scene = importer.ReadFile(
            file_path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_CalcTangentSpace |
            aiProcess_LimitBoneWeights
        );

        if (!scene || !scene->mRootNode) {
            std::cout << "[ARIA Loader] Hata: Assimp skinned model yukleme basarisiz: " << file_path << "\n";
            std::cout << "[Assimp Hata Detayi] " << importer.GetErrorString() << std::endl;
            return std::nullopt;
        }

        raw_skinned_model_data out_model_data;
        out_model_data.debug_path = file_path;
        out_model_data.skeleton.debug_path = file_path;

        // 1. Mesh'leri ve kemik ağırlıklarını/ofset matrislerini işle
        process_assimp_skinned_node(scene->mRootNode, scene, out_model_data);

        // 2. İskelet hiyerarşisini oluştur (Root node ters dönüşümü ve düğüm ağacı)
        glm::mat4 root_transform = assimp_to_glm_mat4(scene->mRootNode->mTransformation);
        out_model_data.skeleton.global_inverse_transform = glm::inverse(root_transform);
        out_model_data.skeleton.root_node = build_skeleton_node(scene->mRootNode, out_model_data.skeleton.bone_mapping);

        // 3. Varsa gömülü animasyon kliplerini ayrıştır
        if (scene->mNumAnimations > 0) {
            out_model_data.animations.reserve(scene->mNumAnimations);
            for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
                raw_animation_data anim = process_assimp_animation(scene->mAnimations[i]);
                anim.debug_path = file_path;
                out_model_data.animations.push_back(std::move(anim));
            }
        }

        return out_model_data;
    }

    std::optional<raw_skeleton_data> asset_loader::load_skeleton_to_cpu(const std::string& file_path) {
        auto model_opt = load_skinned_model_to_cpu(file_path);
        if (!model_opt) return std::nullopt;
        return std::move(model_opt->skeleton);
    }

    std::optional<std::vector<raw_animation_data>> asset_loader::load_animations_to_cpu(const std::string& file_path) {
        if (file_path.empty()) {
            std::cout << "[ARIA Loader] Hata: Animasyon dosya yolu bos.\n";
            return std::nullopt;
        }

        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
        const aiScene* scene = importer.ReadFile(
            file_path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_LimitBoneWeights
        );

        if (!scene) {
            std::cout << "[ARIA Loader] Hata: Assimp animasyon yukleme basarisiz: " << file_path << "\n";
            std::cout << "[Assimp Hata Detayi] " << importer.GetErrorString() << std::endl;
            return std::nullopt;
        }

        if (scene->mNumAnimations == 0) {
            std::cout << "[ARIA Loader] Uyari: Dosyada hicbir animasyon klibi bulunamadi: " << file_path << "\n";
            return std::nullopt;
        }

        std::vector<raw_animation_data> out_animations;
        out_animations.reserve(scene->mNumAnimations);

        for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
            raw_animation_data anim = process_assimp_animation(scene->mAnimations[i]);
            anim.debug_path = file_path;
            out_animations.push_back(std::move(anim));
        }

        return out_animations;
    }

    static std::string sanitize_json_backslashes(const std::string& input) {
        std::string out;
        out.reserve(input.size() + 16);
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '\\') {
                if (i + 1 < input.size()) {
                    char next = input[i + 1];
                    if (next == '"' || next == '\\' || next == '/' || next == 'b' || next == 'f' || next == 'n' || next == 'r' || next == 't' || next == 'u') {
                        out.push_back('\\');
                        out.push_back(next);
                        ++i;
                        continue;
                    }
                }
                out.push_back('/');
            } else {
                out.push_back(input[i]);
            }
        }
        return out;
    }

    std::optional<raw_material_data> asset_loader::load_material_to_cpu(const std::string& file_path) {
        if (file_path.empty()) {
            std::cout << "[ARIA Loader] Hata: Materyal dosya yolu bos.\n";
            return std::nullopt;
        }

        std::string resolved = engine_resources::resolve_path(file_path);
        auto fs_path = qual::path_from_utf8(resolved);
        if (!std::filesystem::exists(fs_path) && project::project_manager::instance().has_active_project()) {
            const auto proj_root = qual::path_from_utf8(project::project_manager::instance().get_project_root());
            if (std::filesystem::exists(proj_root / fs_path)) {
                fs_path = proj_root / fs_path;
                resolved = qual::path_to_utf8(fs_path);
            } else if (std::filesystem::exists(proj_root / "Assets" / fs_path)) {
                fs_path = proj_root / "Assets" / fs_path;
                resolved = qual::path_to_utf8(fs_path);
            }
        }

        json material_json;
        bool loaded_from_vfs = false;
        if (qual::vfs::instance().has_file(resolved)) {
            try {
                const std::string text = qual::vfs::instance().read_text(resolved);
                if (!text.empty()) {
                    try {
                        material_json = json::parse(text);
                    } catch (...) {
                        material_json = json::parse(sanitize_json_backslashes(text));
                    }
                    loaded_from_vfs = true;
                }
            } catch (...) {}
        }

        if (!loaded_from_vfs) {
            std::ifstream file(fs_path);
            if (!file.is_open()) {
                std::cout << "[ARIA Loader] Hata: Materyal dosyasi acilamadi: " << resolved << "\n";
                return std::nullopt;
            }
            std::string raw_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            try {
                material_json = json::parse(raw_content);
            }
            catch (const json::parse_error& e) {
                try {
                    material_json = json::parse(sanitize_json_backslashes(raw_content));
                }
                catch (const json::parse_error& e2) {
                    std::cout << "[ARIA Loader] Hata: JSON ayristirma basarisiz: " << e2.what() << "\n";
                    return std::nullopt;
                }
            }
        }

        try {
            raw_material_data out_raw_data;
            out_raw_data.debug_path = file_path;
            const std::string file_stem = qual::path_to_utf8(qual::path_from_utf8(file_path).stem());
            out_raw_data.name = !file_stem.empty() ? file_stem : material_json.value("name", "Unnamed_Material");

            // Render Modu Değerlendirmesi
            if (material_json.contains("render_mode")) {
                int mode_int = material_json.value("render_mode", 0);
                if (mode_int == 1) {
                    out_raw_data.mode = render_mode::cutout;
                }
                else if (mode_int == 2) {
                    out_raw_data.mode = render_mode::transparent;
                }
                else {
                    out_raw_data.mode = render_mode::opaque;
                }
            }
            else {
                out_raw_data.mode = render_mode::opaque;
            }

            out_raw_data.alpha_threshold = material_json.value("alpha_threshold", 0.1f);

            // Temel Renk (Base Color) Değerlendirmesi
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
                out_raw_data.base_color = glm::vec3(1.0f, 1.0f, 1.0f);
            }

            // Modern PBR Özellikleri (Metallic, Roughness, AO, Emissive)
            out_raw_data.metallic = material_json.value("metallic", 0.0f);
            if (material_json.contains("roughness")) {
                out_raw_data.roughness = material_json.value("roughness", 0.5f);
            }
            else {
                // Geriye dönük uyumluluk: shininess alanından roughness türet
                float s = material_json.value("shininess", 32.0f);
                out_raw_data.roughness = glm::clamp(1.0f - std::sqrt(s / 256.0f), 0.05f, 1.0f);
            }
            out_raw_data.ao = material_json.value("ao", 1.0f);

            if (material_json.contains("emissive_color")) {
                auto& ec = material_json["emissive_color"];
                if (ec.is_array()) {
                    out_raw_data.emissive_color.x = ec.size() > 0 ? ec[0].get<float>() : 0.0f;
                    out_raw_data.emissive_color.y = ec.size() > 1 ? ec[1].get<float>() : 0.0f;
                    out_raw_data.emissive_color.z = ec.size() > 2 ? ec[2].get<float>() : 0.0f;
                }
                else if (ec.is_object()) {
                    out_raw_data.emissive_color.x = ec.value("x", 0.0f);
                    out_raw_data.emissive_color.y = ec.value("y", 0.0f);
                    out_raw_data.emissive_color.z = ec.value("z", 0.0f);
                }
            }
            else {
                out_raw_data.emissive_color = glm::vec3(0.0f);
            }
            out_raw_data.emissive_intensity = material_json.value("emissive_intensity", 0.0f);

            // Standart ve Eski Nesil (Legacy) Özellikler
            out_raw_data.specular_strength = material_json.value("specular_strength", 0.5f);
            out_raw_data.shininess = material_json.value("shininess", 32.0f);
            out_raw_data.normal_strength = material_json.value("normal_strength", 1.0f);

            // Doku Ölçeklendirmesi (Texture Scale) Değerlendirmesi
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
                out_raw_data.texture_scale = glm::vec2(1.0f, 1.0f);
            }

            // JSON içindeki string formatlı GUID'leri stduuid yapısına güvenle çeviren yerel fonksiyon
            auto parse_guid_helper = [](const json& j, const std::string& key) -> aria::asset_guid {
                std::string guid_str = j.value(key, "");
                if (guid_str.empty()) return {};

                auto parsed = uuids::uuid::from_string(guid_str);
                return parsed ? *parsed : aria::asset_guid{};
                };

            // Referans GUID Atamaları (PBR Doku Haritaları Dahil)
            out_raw_data.shader_guid = parse_guid_helper(material_json, "shader_guid");
            out_raw_data.diffuse_texture_guid = parse_guid_helper(material_json, "diffuse_texture_guid");
            out_raw_data.normal_texture_guid = parse_guid_helper(material_json, "normal_texture_guid");
            out_raw_data.metallic_texture_guid = parse_guid_helper(material_json, "metallic_texture_guid");
            out_raw_data.roughness_texture_guid = parse_guid_helper(material_json, "roughness_texture_guid");
            out_raw_data.ao_texture_guid = parse_guid_helper(material_json, "ao_texture_guid");
            out_raw_data.emissive_texture_guid = parse_guid_helper(material_json, "emissive_texture_guid");
            out_raw_data.specular_texture_guid = parse_guid_helper(material_json, "specular_texture_guid");

            return out_raw_data;
        }
        catch (const nlohmann::json::exception& e) {
            std::cout << "[ARIA Loader] Hata: JSON Tip Uyumsuzlugu (Runtime): " << e.what() << "\n";
            return std::nullopt;
        }
    }

    std::optional<raw_shader_data> asset_loader::load_shader_to_cpu(const std::string& file_path) {
        std::string resolved = engine_resources::resolve_path(file_path);
        std::string vert_path = resolved + ".vert";
        std::string frag_path = resolved + ".frag";

        raw_shader_data raw;
        raw.debug_path = resolved;

        if (qual::vfs::instance().has_file(vert_path) && qual::vfs::instance().has_file(frag_path)) {
            raw.vertex_source = qual::vfs::instance().read_text(vert_path);
            raw.fragment_source = qual::vfs::instance().read_text(frag_path);
            return raw;
        }

        std::ifstream vFile(qual::path_from_utf8(vert_path));
        std::ifstream fFile(qual::path_from_utf8(frag_path));

        if (!vFile.is_open()) {
            std::cout << "[ARIA Loader] Hata: Vertex shader dosyasi bulunamadi: " << vert_path << std::endl;
        }
        if (!fFile.is_open()) {
            std::cout << "[ARIA Loader] Hata: Fragment shader dosyasi bulunamadi: " << frag_path << std::endl;
        }

        if (!vFile.is_open() || !fFile.is_open()) {
            return std::nullopt;
        }

        std::stringstream vStr, fStr;
        vStr << vFile.rdbuf();
        fStr << fFile.rdbuf();

        raw.vertex_source = vStr.str();
        raw.fragment_source = fStr.str();

        vFile.close();
        fFile.close();

        return raw;
    }

    std::optional<raw_audio_data> asset_loader::load_audio_to_cpu(const std::string& file_path) {
        std::string resolved = engine_resources::resolve_path(file_path);
        auto fs_path = qual::path_from_utf8(resolved);
        if (!std::filesystem::exists(fs_path)) {
            std::cerr << "[ARIA Loader] Hata: Ses dosyasi bulunamadi: " << resolved << "\n";
            return std::nullopt;
        }

        ma_decoder decoder;
        ma_result result = MA_ERROR;
        std::vector<uint8_t> vfs_audio_bytes;

        if (qual::vfs::instance().has_file(file_path)) {
            vfs_audio_bytes = qual::vfs::instance().read_bytes(resolved);
            if (!vfs_audio_bytes.empty()) {
                result = ma_decoder_init_memory(vfs_audio_bytes.data(), vfs_audio_bytes.size(), nullptr, &decoder);
            }
        }
        if (result != MA_SUCCESS) {
#if defined(_WIN32)
            result = ma_decoder_init_file_w(fs_path.wstring().c_str(), nullptr, &decoder);
#else
            result = ma_decoder_init_file(resolved.c_str(), nullptr, &decoder);
#endif
        }
        if (result != MA_SUCCESS) {
            std::cerr << "[ARIA Loader] Hata: Ses dosyasi okunamadi: " << resolved << " (Kod: " << result << ")\n";
            return std::nullopt;
        }

        raw_audio_data raw;
        raw.filepath = resolved;
        raw.channels = decoder.outputChannels;
        raw.sample_rate = decoder.outputSampleRate;

        ma_uint64 totalFrames = 0;
        if (ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames) == MA_SUCCESS && decoder.outputSampleRate > 0) {
            raw.duration_seconds = static_cast<float>(totalFrames) / static_cast<float>(decoder.outputSampleRate);
        } else {
            raw.duration_seconds = 0.0f;
        }

        ma_decoder_uninit(&decoder);
        return raw;
    }


        static bool is_ps1_retro_mode_active() {
        if (auto* eCtx = gameplay::context::get_engine_context()) {
            return eCtx->renderSettings.enable_ps1_mode;
        }
        if (project::project_manager::instance().has_active_project()) {
            return project::project_manager::instance().get_descriptor().rendering.enable_ps1_mode;
        }
        return false;
    }

    std::vector<asset_loader::auto_extracted_material> asset_loader::extract_and_generate_materials_for_model(
        const std::string& model_file_path,
        class system* aria_sys
    ) {
        std::vector<auto_extracted_material> results;
        if (model_file_path.empty()) return results;

        std::filesystem::path model_path = qual::path_from_utf8(model_file_path);
        if (!std::filesystem::exists(model_path)) return results;

        std::string ext = qual::path_to_utf8(model_path.extension());
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".glb" && ext != ".gltf" && ext != ".fbx" && ext != ".obj" && ext != ".dae") {
            return results;
        }

        std::filesystem::path model_dir = model_path.parent_path();
        std::string model_stem = qual::path_to_utf8(model_path.stem());

        // 1. Hedef Dizinlerin Belirlenmesi:
        // Modelin bulundugu dizinde derli toplu klasorler (Textures ve Materials)
        std::filesystem::path tex_dir = model_dir / "Textures";
        std::filesystem::path mat_dir = (std::filesystem::exists(model_dir / "Materials"))
            ? (model_dir / "Materials")
            : model_dir;

        // Eger onceden olusturulmus bir materyal varsa dogrudan onu dondur (var olan ayarlari koru)
        std::filesystem::path direct_mat = mat_dir / qual::path_from_utf8(model_stem + "_Material.json");
        std::filesystem::path alt_mat = model_dir / "Materials" / qual::path_from_utf8(model_stem + "_Material.json");
        std::filesystem::path plain_mat = model_dir / qual::path_from_utf8(model_stem + ".json");

        std::filesystem::path existing_mat_path;
        if (std::filesystem::exists(direct_mat)) existing_mat_path = direct_mat;
        else if (std::filesystem::exists(alt_mat)) existing_mat_path = alt_mat;
        else if (std::filesystem::exists(plain_mat)) existing_mat_path = plain_mat;

        const bool ps1_active = is_ps1_retro_mode_active();

        if (!existing_mat_path.empty()) {
            std::string path_u8 = qual::path_to_utf8(existing_mat_path);
            aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(path_u8);
            if (aria_sys && !guid.is_nil()) {
                aria_sys->register_asset_path(guid, path_u8);
            }

            // Retro/PS1 modu aciksa ve mevcut materyal varsayilan standart shader'da ise PS1 shader'a yukselt
            if (ps1_active) {
                try {
                    std::ifstream in(existing_mat_path);
                    if (in.is_open()) {
                        nlohmann::json exJson;
                        in >> exJson;
                        in.close();
                        std::string curSh = exJson.value("shader_path", "");
                        std::string defShGuid = uuids::to_string(engine_resources::builtin_guid::default_shader);
                        if (curSh.empty() || curSh == "@engine/Shaders/default_shader" || exJson.value("shader_guid", "") == defShGuid) {
                            exJson["shader_guid"] = uuids::to_string(engine_resources::builtin_guid::ps1_shader);
                            exJson["shader_path"] = "@engine/Shaders/PS1/ps1_shader";
                            std::ofstream out(existing_mat_path);
                            if (out.is_open()) {
                                out << exJson.dump(2);
                            }
                            if (aria_sys && !guid.is_nil()) {
                                aria_sys->registry()->reload_material(guid, path_u8);
                            }
                        }
                    }
                } catch (...) {}
            }

            results.push_back({ qual::path_to_utf8(existing_mat_path.stem()), path_u8, guid });
            return results;
        }

        // Assimp ile modeli ac ve materyal/doku tanimlarini cozumle
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            model_file_path,
            aiProcess_Triangulate | aiProcess_GenNormals
        );
        if (!scene) {
            return results;
        }

        std::error_code ec;
        std::filesystem::create_directories(tex_dir, ec);

        // Gomulu veya bagil disk dokusunu tespit edip kaydeden lambda
        auto resolve_or_extract_texture = [&](aiMaterial* mat, aiTextureType type, const std::string& suffix) -> std::pair<std::string, aria::asset_guid> {
            aiString texPath;
            if (mat->GetTexture(type, 0, &texPath) != AI_SUCCESS || texPath.length == 0) {
                return { "", aria::asset_guid{} };
            }

            const char* pathStr = texPath.C_Str();

            // A) Gomulu Doku (GLB veya embedded FBX)
            const aiTexture* embedded = scene->GetEmbeddedTexture(pathStr);
            if (!embedded && pathStr[0] == '*') {
                int idx = std::atoi(pathStr + 1);
                if (idx >= 0 && idx < static_cast<int>(scene->mNumTextures)) {
                    embedded = scene->mTextures[idx];
                }
            }

            if (embedded) {
                std::string formatHint = (embedded->achFormatHint[0] != '\0') ? embedded->achFormatHint : "png";
                std::transform(formatHint.begin(), formatHint.end(), formatHint.begin(), ::tolower);
                if (!formatHint.empty() && formatHint.front() == '.') formatHint.erase(0, 1);
                if (formatHint.empty()) formatHint = "png";

                std::string outFilename = model_stem + suffix + "." + formatHint;
                std::filesystem::path outPath = tex_dir / qual::path_from_utf8(outFilename);

                if (!std::filesystem::exists(outPath)) {
                    if (embedded->mHeight == 0 && embedded->mWidth > 0) {
                        // Sikistirilmis dosya verisi (PNG, JPG)
                        std::ofstream out(outPath, std::ios::binary);
                        if (out.is_open()) {
                            out.write(reinterpret_cast<const char*>(embedded->pcData), embedded->mWidth);
                            out.close();
                        }
                    }
                }

                if (std::filesystem::exists(outPath)) {
                    std::string pathU8 = qual::path_to_utf8(outPath);
                    aria::asset_guid g = asset_pipeline::get_or_create_meta_guid(pathU8);
                    if (aria_sys && !g.is_nil()) {
                        aria_sys->register_asset_path(g, pathU8);
                    }
                    return { pathU8, g };
                }
            }

            // B) Harici Doku Referansi
            std::filesystem::path rawTexP = qual::path_from_utf8(pathStr);
            std::string rawFilename = qual::path_to_utf8(rawTexP.filename());

            std::filesystem::path candidates[] = {
                tex_dir / qual::path_from_utf8(rawFilename),
                model_dir / qual::path_from_utf8(rawFilename),
                model_dir / rawTexP,
                model_dir.parent_path() / "Textures" / qual::path_from_utf8(rawFilename)
            };

            for (const auto& cand : candidates) {
                if (std::filesystem::exists(cand)) {
                    std::string pathU8 = qual::path_to_utf8(cand);
                    aria::asset_guid g = asset_pipeline::get_or_create_meta_guid(pathU8);
                    if (aria_sys && !g.is_nil()) {
                        aria_sys->register_asset_path(g, pathU8);
                    }
                    return { pathU8, g };
                }
            }

            return { "", aria::asset_guid{} };
        };

        unsigned int matCount = scene->mNumMaterials;
        if (matCount == 0) matCount = 1;

        for (unsigned int m = 0; m < matCount; ++m) {
            aiMaterial* mat = (scene->mNumMaterials > m) ? scene->mMaterials[m] : nullptr;

            std::string matStem = model_stem + "_Material";
            if (mat) {
                aiString rawMatName;
                if (mat->Get(AI_MATKEY_NAME, rawMatName) == AI_SUCCESS && rawMatName.length > 0) {
                    std::string candidateName = rawMatName.C_Str();
                    if (candidateName != "DefaultMaterial" && candidateName != "default" && candidateName != "Material") {
                        matStem = model_stem + "_" + candidateName;
                    }
                }
            }
            if (matCount > 1) {
                matStem += "_" + std::to_string(m);
            }

            // Dokulari ara/cikart
            auto [diffPath, diffGuid] = mat ? resolve_or_extract_texture(mat, aiTextureType_DIFFUSE, "_Albedo") : std::make_pair(std::string{}, aria::asset_guid{});
            if (diffPath.empty() && mat) {
                std::tie(diffPath, diffGuid) = resolve_or_extract_texture(mat, aiTextureType_BASE_COLOR, "_Albedo");
            }

            auto [normPath, normGuid] = mat ? resolve_or_extract_texture(mat, aiTextureType_NORMALS, "_Normal") : std::make_pair(std::string{}, aria::asset_guid{});
            if (normPath.empty() && mat) {
                std::tie(normPath, normGuid) = resolve_or_extract_texture(mat, aiTextureType_HEIGHT, "_Normal");
            }

            auto [metalPath, metalGuid] = mat ? resolve_or_extract_texture(mat, aiTextureType_METALNESS, "_Metallic") : std::make_pair(std::string{}, aria::asset_guid{});
            if (metalPath.empty() && mat) {
                std::tie(metalPath, metalGuid) = resolve_or_extract_texture(mat, aiTextureType_DIFFUSE_ROUGHNESS, "_Roughness");
            }
            if (metalPath.empty() && mat) {
                std::tie(metalPath, metalGuid) = resolve_or_extract_texture(mat, aiTextureType_UNKNOWN, "_PBR");
            }

            auto [emissPath, emissGuid] = mat ? resolve_or_extract_texture(mat, aiTextureType_EMISSIVE, "_Emissive") : std::make_pair(std::string{}, aria::asset_guid{});

            // Renk ve PBR katsayilari
            aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (mat) {
                if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) != AI_SUCCESS) {
                    mat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
                }
            }

            float metallic = 0.0f;
            float roughness = 0.5f;
            if (mat) {
                mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
                mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            }

            std::filesystem::path finalMatPath = mat_dir / qual::path_from_utf8(matStem + ".json");
            std::string finalMatPathU8 = qual::path_to_utf8(finalMatPath);

            nlohmann::json mat_json;
            mat_json["type"] = "material";
            mat_json["name"] = matStem;
            mat_json["base_color"] = { baseColor.r, baseColor.g, baseColor.b, baseColor.a };
            mat_json["metallic"] = metallic;
            mat_json["roughness"] = roughness;
            mat_json["ao"] = 1.0f;
            mat_json["emissive_color"] = { 0.0f, 0.0f, 0.0f };
            mat_json["emissive_intensity"] = 0.0f;
            mat_json["specular_strength"] = 0.5f;
            mat_json["shininess"] = std::pow(1.0f - roughness, 2.0f) * 256.0f;
            mat_json["normal_strength"] = 1.0f;
            mat_json["texture_scale"] = { 1.0f, 1.0f };
            mat_json["render_mode"] = 0;
            mat_json["alpha_threshold"] = 0.1f;
            if (ps1_active) {
                mat_json["shader_guid"] = uuids::to_string(engine_resources::builtin_guid::ps1_shader);
                mat_json["shader_path"] = "@engine/Shaders/PS1/ps1_shader";
            } else {
                mat_json["shader_guid"] = uuids::to_string(engine_resources::builtin_guid::default_shader);
                mat_json["shader_path"] = "@engine/Shaders/default_shader";
            }
            mat_json["diffuse_texture_guid"] = diffGuid.is_nil() ? "00000000-0000-0000-0000-000000000000" : uuids::to_string(diffGuid);
            mat_json["normal_texture_guid"] = normGuid.is_nil() ? "00000000-0000-0000-0000-000000000000" : uuids::to_string(normGuid);
            mat_json["metallic_texture_guid"] = metalGuid.is_nil() ? "00000000-0000-0000-0000-000000000000" : uuids::to_string(metalGuid);
            mat_json["roughness_texture_guid"] = metalGuid.is_nil() ? "00000000-0000-0000-0000-000000000000" : uuids::to_string(metalGuid);
            mat_json["ao_texture_guid"] = "00000000-0000-0000-0000-000000000000";
            mat_json["emissive_texture_guid"] = emissGuid.is_nil() ? "00000000-0000-0000-0000-000000000000" : uuids::to_string(emissGuid);
            mat_json["specular_texture_guid"] = "00000000-0000-0000-0000-000000000000";

            std::ofstream out(finalMatPath);
            if (out.is_open()) {
                out << mat_json.dump(2);
                out.close();
                std::cout << "[ARIA] Model icin otomatik PBR materyal olusturuldu: " << finalMatPathU8 << std::endl;
            }

            aria::asset_guid matGuid = asset_pipeline::get_or_create_meta_guid(finalMatPathU8);
            if (aria_sys && !matGuid.is_nil()) {
                aria_sys->register_asset_path(matGuid, finalMatPathU8);
            }

            results.push_back({ matStem, finalMatPathU8, matGuid });
        }

        return results;
    }

} // namespace aria
