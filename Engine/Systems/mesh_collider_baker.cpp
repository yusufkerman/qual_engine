/**
 * @file mesh_collider_baker.cpp
 * @brief Hiyerarşik alt ağları toplayan, basitleştiren ve Jolt Physics Shape üreten servis uygulaması.
 */

#include "mesh_collider_baker.h"
#include "../ECS/registry.h"
#include "../ECS/transform_utils.h"
#include "../Engine/Assets/aria.h"
#include "../Engine/Assets/asset_loader.h"
#include "../Core/Logger/logger.h"

#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace qual::physics
{
    namespace
    {
        void generate_cube_geometry(std::vector<glm::vec3>& outVerts, std::vector<uint32_t>& outIndices)
        {
            const float h = 0.5f;
            outVerts = {
                { -h, -h, -h }, {  h, -h, -h }, {  h,  h, -h }, { -h,  h, -h },
                { -h, -h,  h }, {  h, -h,  h }, {  h,  h,  h }, { -h,  h,  h }
            };
            outIndices = {
                0, 2, 1, 0, 3, 2, // Arka
                4, 5, 6, 4, 6, 7, // On
                0, 1, 5, 0, 5, 4, // Alt
                2, 3, 7, 2, 7, 6, // Ust
                0, 4, 7, 0, 7, 3, // Sol
                1, 2, 6, 1, 6, 5  // Sag
            };
        }

        void generate_sphere_geometry(float radius, int sectors, int stacks, std::vector<glm::vec3>& outVerts, std::vector<uint32_t>& outIndices)
        {
            outVerts.clear();
            outIndices.clear();

            for (int i = 0; i <= stacks; ++i)
            {
                const float stackAngle = 3.1415926535f / 2.0f - static_cast<float>(i) * (3.1415926535f / static_cast<float>(stacks));
                const float xy = radius * std::cos(stackAngle);
                const float z = radius * std::sin(stackAngle);

                for (int j = 0; j <= sectors; ++j)
                {
                    const float sectorAngle = static_cast<float>(j) * (2.0f * 3.1415926535f / static_cast<float>(sectors));
                    const float x = xy * std::cos(sectorAngle);
                    const float y = xy * std::sin(sectorAngle);
                    outVerts.push_back(glm::vec3(x, z, y));
                }
            }

            for (int i = 0; i < stacks; ++i)
            {
                int k1 = i * (sectors + 1);
                int k2 = k1 + sectors + 1;

                for (int j = 0; j < sectors; ++j, ++k1, ++k2)
                {
                    if (i != 0)
                    {
                        outIndices.push_back(k1);
                        outIndices.push_back(k2);
                        outIndices.push_back(k1 + 1);
                    }
                    if (i != (stacks - 1))
                    {
                        outIndices.push_back(k1 + 1);
                        outIndices.push_back(k2);
                        outIndices.push_back(k2 + 1);
                    }
                }
            }
        }

        void append_transformed_submesh(
            const std::vector<glm::vec3>& localPositions,
            const std::vector<uint32_t>& localIndices,
            const glm::mat4& relMat,
            std::vector<glm::vec3>& outRawVertices,
            std::vector<uint32_t>& outRawIndices,
            uint32_t& outSubmeshCount
        )
        {
            if (localPositions.empty() || localIndices.size() < 3)
            {
                return;
            }

            const uint32_t baseVertexIndex = static_cast<uint32_t>(outRawVertices.size());
            outRawVertices.reserve(outRawVertices.size() + localPositions.size());

            for (const auto& pos : localPositions)
            {
                const glm::vec4 transformed = relMat * glm::vec4(pos, 1.0f);
                outRawVertices.push_back(glm::vec3(transformed));
            }

            outRawIndices.reserve(outRawIndices.size() + localIndices.size());
            for (size_t i = 0; i + 2 < localIndices.size(); i += 3)
            {
                const uint32_t i0 = localIndices[i];
                const uint32_t i1 = localIndices[i + 1];
                const uint32_t i2 = localIndices[i + 2];
                if (i0 < localPositions.size() && i1 < localPositions.size() && i2 < localPositions.size())
                {
                    outRawIndices.push_back(baseVertexIndex + i0);
                    outRawIndices.push_back(baseVertexIndex + i1);
                    outRawIndices.push_back(baseVertexIndex + i2);
                }
            }

            outSubmeshCount++;
        }

        void ensure_model_collision_data(const aria::model_asset* modelData, aria::system* ariaSys)
        {
            if (!modelData || modelData->sub_meshes.empty())
            {
                return;
            }

            bool anyMissing = false;
            for (const auto& sm : modelData->sub_meshes)
            {
                if (sm.collision_positions.empty() || sm.collision_indices.empty())
                {
                    anyMissing = true;
                    break;
                }
            }

            if (!anyMissing)
            {
                return;
            }

            // 1. Oncelikle bellekteki raw_data.sub_meshes verisini kontrol et
            if (modelData->raw_data.sub_meshes.size() == modelData->sub_meshes.size())
            {
                bool populatedFromRaw = true;
                for (size_t i = 0; i < modelData->sub_meshes.size(); ++i)
                {
                    auto& dst = const_cast<aria::mesh_asset&>(modelData->sub_meshes[i]);
                    const auto& src = modelData->raw_data.sub_meshes[i];
                    if (!src.vertices.empty() && !src.indices.empty())
                    {
                        dst.collision_positions.clear();
                        dst.collision_positions.reserve(src.vertices.size());
                        for (const auto& v : src.vertices)
                        {
                            dst.collision_positions.push_back(v.position);
                        }
                        dst.collision_indices = src.indices;
                    }
                    else
                    {
                        populatedFromRaw = false;
                    }
                }
                if (populatedFromRaw)
                {
                    return;
                }
            }

            // 2. Fallback: Diskten tek seferde tum submesh'leri yukle
            if (ariaSys)
            {
                const std::string path = ariaSys->resolve_asset_path(modelData->guid);
                if (!path.empty())
                {
                    auto rawOpt = aria::asset_loader::load_model_to_cpu(path);
                    if (rawOpt)
                    {
                        const size_t count = std::min(modelData->sub_meshes.size(), rawOpt->sub_meshes.size());
                        for (size_t i = 0; i < count; ++i)
                        {
                            auto& dst = const_cast<aria::mesh_asset&>(modelData->sub_meshes[i]);
                            const auto& src = rawOpt->sub_meshes[i];
                            dst.collision_positions.clear();
                            dst.collision_positions.reserve(src.vertices.size());
                            for (const auto& v : src.vertices)
                            {
                                dst.collision_positions.push_back(v.position);
                            }
                            dst.collision_indices = src.indices;
                        }
                    }
                }
            }
        }

        void collect_entity_meshes(
            entity current,
            const glm::mat4& rootWorld,
            registry& reg,
            aria::system* ariaSys,
            std::vector<glm::vec3>& outRawVertices,
            std::vector<uint32_t>& outRawIndices,
            uint32_t& outSubmeshCount
        )
        {
            // Hiyerarsideki alt cocuklari onceden tespit et
            std::vector<entity> children;
            const auto* hier = reg.get_component<hierarchy_component>(current);
            if (hier && hier->first_child != 0)
            {
                entity child = hier->first_child;
                int safety = 0;
                while (child != 0 && safety++ < 10000)
                {
                    children.push_back(child);
                    const auto* childHier = reg.get_component<hierarchy_component>(child);
                    child = childHier ? childHier->next_sibling : 0;
                }
            }

            if (children.empty())
            {
                for (auto ent : reg.view<hierarchy_component>())
                {
                    const entity e = static_cast<entity>(ent);
                    const auto* candHier = reg.get_component<hierarchy_component>(e);
                    if (candHier && candHier->parent == current && e != current)
                    {
                        children.push_back(e);
                    }
                }
            }

            if (reg.has_component<mesh_component>(current))
            {
                const auto* meshComp = reg.get_component<mesh_component>(current);
                const glm::mat4 childWorld = ecs_transform::get_world_matrix(current, reg);
                const glm::mat4 relMat = glm::inverse(rootWorld) * childWorld;

                if (meshComp->source == mesh_source_type::primitive_cube)
                {
                    std::vector<glm::vec3> localPositions;
                    std::vector<uint32_t> localIndices;
                    generate_cube_geometry(localPositions, localIndices);
                    append_transformed_submesh(localPositions, localIndices, relMat, outRawVertices, outRawIndices, outSubmeshCount);
                }
                else if (meshComp->source == mesh_source_type::primitive_sphere)
                {
                    std::vector<glm::vec3> localPositions;
                    std::vector<uint32_t> localIndices;
                    generate_sphere_geometry(0.5f, 20, 10, localPositions, localIndices);
                    append_transformed_submesh(localPositions, localIndices, relMat, outRawVertices, outRawIndices, outSubmeshCount);
                }
                else if (ariaSys && meshComp->model_handle.is_valid())
                {
                    const auto* modelData = ariaSys->get_model_data(meshComp->model_handle);
                    if (modelData && !modelData->sub_meshes.empty())
                    {
                        ensure_model_collision_data(modelData, ariaSys);

                        // Eger tek bir varlik uzerine dogrudan cok-parcali bir model atanmissa (cocuk entity'leri yoksa),
                        // modeldeki tum submesh'leri kendi local_transform matrisleriyle topla.
                        bool hasChildWithSameModel = false;
                        for (entity ch : children)
                        {
                            if (const auto* chMc = reg.get_component<mesh_component>(ch))
                            {
                                if (chMc->model_handle == meshComp->model_handle)
                                {
                                    hasChildWithSameModel = true;
                                    break;
                                }
                            }
                        }

                        if (!hasChildWithSameModel && children.empty() && modelData->sub_meshes.size() > 1 && meshComp->sub_mesh_index == 0)
                        {
                            for (const auto& sub : modelData->sub_meshes)
                            {
                                append_transformed_submesh(
                                    sub.collision_positions,
                                    sub.collision_indices,
                                    relMat * sub.local_transform,
                                    outRawVertices,
                                    outRawIndices,
                                    outSubmeshCount
                                );
                            }
                        }
                        else if (meshComp->sub_mesh_index < modelData->sub_meshes.size())
                        {
                            const auto& sub = modelData->sub_meshes[meshComp->sub_mesh_index];
                            append_transformed_submesh(
                                sub.collision_positions,
                                sub.collision_indices,
                                relMat,
                                outRawVertices,
                                outRawIndices,
                                outSubmeshCount
                            );
                        }
                    }
                }
            }

            // Iskeletli (Skinned) Mesh destegi
            if (reg.has_component<skinned_mesh_component>(current))
            {
                const auto* smc = reg.get_component<skinned_mesh_component>(current);
                if (ariaSys && smc && smc->model_handle.is_valid())
                {
                    const auto* skinnedData = ariaSys->get_skinned_model_data(smc->model_handle);
                    if (skinnedData && smc->sub_mesh_index < skinnedData->sub_meshes.size())
                    {
                        const glm::mat4 childWorld = ecs_transform::get_world_matrix(current, reg);
                        const glm::mat4 relMat = glm::inverse(rootWorld) * childWorld;
                        const auto& sub = skinnedData->sub_meshes[smc->sub_mesh_index];
                        append_transformed_submesh(
                            sub.collision_positions,
                            sub.collision_indices,
                            relMat,
                            outRawVertices,
                            outRawIndices,
                            outSubmeshCount
                        );
                    }
                }
            }

            for (entity child : children)
            {
                collect_entity_meshes(
                    child,
                    rootWorld,
                    reg,
                    ariaSys,
                    outRawVertices,
                    outRawIndices,
                    outSubmeshCount
                );
            }
        }

        /**
         * @brief Full (1:1) modunda hiçbir üçgeni silmeden veya sadeleştirmeden yalnızca UV/Normal dikişlerindeki
         * birebir çakışık köşe noktalarını (exact duplicates) birleştirir ve geçersiz (sıfır alanlı) indisleri temizler.
         */
        void weld_exact_vertices(
            const std::vector<glm::vec3>& inVertices,
            const std::vector<uint32_t>& inIndices,
            std::vector<glm::vec3>& outVertices,
            std::vector<uint32_t>& outIndices
        )
        {
            outVertices.clear();
            outIndices.clear();

            if (inVertices.empty() || inIndices.size() < 3)
            {
                return;
            }

            struct QuantizedPos
            {
                int64_t x, y, z;
                bool operator==(const QuantizedPos& o) const
                {
                    return x == o.x && y == o.y && z == o.z;
                }
            };

            struct QuantizedHash
            {
                size_t operator()(const QuantizedPos& p) const
                {
                    size_t h1 = std::hash<int64_t>()(p.x);
                    size_t h2 = std::hash<int64_t>()(p.y);
                    size_t h3 = std::hash<int64_t>()(p.z);
                    return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2)) ^ (h3 * 0xc4ceb9fe1a85ec53ULL);
                }
            };

            constexpr float weldScale = 100000.0f; // 1e-5m hassasiyet: yalnizca birebir ayni konumda olan dikis koselerini birlestirir
            std::unordered_map<QuantizedPos, uint32_t, QuantizedHash> uniqueMap;
            uniqueMap.reserve(inVertices.size());

            std::vector<uint32_t> remap(inVertices.size());
            outVertices.reserve(inVertices.size());

            for (size_t i = 0; i < inVertices.size(); ++i)
            {
                const glm::vec3& v = inVertices[i];
                const QuantizedPos qp{
                    static_cast<int64_t>(std::llround(static_cast<double>(v.x) * weldScale)),
                    static_cast<int64_t>(std::llround(static_cast<double>(v.y) * weldScale)),
                    static_cast<int64_t>(std::llround(static_cast<double>(v.z) * weldScale))
                };

                auto [it, inserted] = uniqueMap.try_emplace(qp, static_cast<uint32_t>(outVertices.size()));
                if (inserted)
                {
                    outVertices.push_back(v);
                }
                remap[i] = it->second;
            }

            outIndices.reserve(inIndices.size());
            for (size_t i = 0; i + 2 < inIndices.size(); i += 3)
            {
                if (inIndices[i] >= inVertices.size() ||
                    inIndices[i + 1] >= inVertices.size() ||
                    inIndices[i + 2] >= inVertices.size())
                {
                    continue;
                }

                const uint32_t i0 = remap[inIndices[i]];
                const uint32_t i1 = remap[inIndices[i + 1]];
                const uint32_t i2 = remap[inIndices[i + 2]];

                if (i0 == i1 || i1 == i2 || i0 == i2)
                {
                    continue;
                }

                outIndices.push_back(i0);
                outIndices.push_back(i1);
                outIndices.push_back(i2);
            }

            if (outIndices.empty())
            {
                outVertices = inVertices;
                outIndices = inIndices;
            }
        }
    } // anonymous namespace

    void mesh_collider_baker::simplify_mesh(
        const std::vector<glm::vec3>& inVertices,
        const std::vector<uint32_t>& inIndices,
        float cellDivisor,
        std::vector<glm::vec3>& outVertices,
        std::vector<uint32_t>& outIndices
    )
    {
        outVertices.clear();
        outIndices.clear();

        if (inVertices.size() < 12 || inIndices.size() < 12 || cellDivisor <= 1.0f)
        {
            outVertices = inVertices;
            outIndices = inIndices;
            return;
        }

        // 1. Sinirlayici kutuyu (AABB) hesapla
        glm::vec3 minB = inVertices[0];
        glm::vec3 maxB = inVertices[0];
        for (const auto& v : inVertices)
        {
            minB = glm::min(minB, v);
            maxB = glm::max(maxB, v);
        }

        const glm::vec3 extent = maxB - minB;
        const float maxDim = std::max({ extent.x, extent.y, extent.z });
        if (maxDim < 0.0001f)
        {
            outVertices = inVertices;
            outIndices = inIndices;
            return;
        }

        // Yuzey yogunluguna duyarli hucre boyutu: hem kucuk nesnelerde hem Sponza gibi buyuk sahnelerde
        // hedef sadelestirme oranini (~75%, ~40%, ~20%) korur.
        const float surfaceArea = 2.0f * (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x);
        const size_t triCount = std::max<size_t>(1, inIndices.size() / 3);
        const float avgTriSize = std::sqrt(std::max(0.0001f, surfaceArea) / static_cast<float>(triCount));

        // cellDivisor: High=85, Medium=42, Low=20 -> yoğunluk çarpanına dönüştür
        const float densityFactor = 85.0f / std::max(5.0f, cellDivisor);
        const float adaptiveCell = avgTriSize * densityFactor * 0.60f;
        const float boxCell = maxDim / (cellDivisor * 2.5f);
        const float cellSize = std::clamp(std::min(adaptiveCell, boxCell), maxDim * 0.0005f, maxDim / 12.0f);
        const float invCell = 1.0f / std::max(cellSize, 1e-5f);

        auto calc_key = [minB, invCell](const glm::vec3& v) -> uint64_t {
            const int64_t gx = static_cast<int64_t>(std::floor((v.x - minB.x) * invCell));
            const int64_t gy = static_cast<int64_t>(std::floor((v.y - minB.y) * invCell));
            const int64_t gz = static_cast<int64_t>(std::floor((v.z - minB.z) * invCell));
            return (uint64_t(gx & 0x1FFFFF)) |
                   (uint64_t(gy & 0x1FFFFF) << 21) |
                   (uint64_t(gz & 0x1FFFFF) << 42);
        };

        // 2. Vertex Clustering
        struct ClusterInfo
        {
            glm::vec3 sumPos{ 0.0f };
            uint32_t count = 0;
            uint32_t remappedIndex = 0;
        };

        std::unordered_map<uint64_t, ClusterInfo> grid;
        grid.reserve(inVertices.size() / 2);

        for (const auto& v : inVertices)
        {
            const uint64_t k = calc_key(v);
            auto& c = grid[k];
            c.sumPos += v;
            c.count++;
        }

        outVertices.reserve(grid.size());
        uint32_t nextIdx = 0;
        for (auto& [key, c] : grid)
        {
            c.remappedIndex = nextIdx++;
            outVertices.push_back(c.sumPos / static_cast<float>(c.count));
        }

        std::vector<uint32_t> vertToNew(inVertices.size());
        for (size_t i = 0; i < inVertices.size(); ++i)
        {
            const uint64_t k = calc_key(inVertices[i]);
            vertToNew[i] = grid[k].remappedIndex;
        }

        // 3. Ucgenleri yeniden haritalandir ve dejenere/duplicate olanlari filtrele
        struct TriKey
        {
            uint32_t i0, i1, i2;
            bool operator==(const TriKey& o) const
            {
                return i0 == o.i0 && i1 == o.i1 && i2 == o.i2;
            }
        };

        struct TriHash
        {
            size_t operator()(const TriKey& k) const
            {
                return std::hash<uint32_t>()(k.i0) ^
                      (std::hash<uint32_t>()(k.i1) << 1) ^
                      (std::hash<uint32_t>()(k.i2) << 2);
            }
        };

        std::unordered_set<TriKey, TriHash> seenTriangles;
        seenTriangles.reserve(inIndices.size() / 3);
        outIndices.reserve(inIndices.size());

        for (size_t i = 0; i + 2 < inIndices.size(); i += 3)
        {
            const uint32_t idx0 = vertToNew[inIndices[i]];
            const uint32_t idx1 = vertToNew[inIndices[i + 1]];
            const uint32_t idx2 = vertToNew[inIndices[i + 2]];

            // Kose noktalari ayni hucreye dusup coken ucgenleri ele
            if (idx0 == idx1 || idx1 == idx2 || idx0 == idx2)
            {
                continue;
            }

            // Yon bagimsiz tekil karsilastirma anahtari
            uint32_t s0 = idx0, s1 = idx1, s2 = idx2;
            if (s0 > s1) std::swap(s0, s1);
            if (s1 > s2) std::swap(s1, s2);
            if (s0 > s1) std::swap(s0, s1);

            if (seenTriangles.insert({ s0, s1, s2 }).second)
            {
                outIndices.push_back(idx0);
                outIndices.push_back(idx1);
                outIndices.push_back(idx2);
            }
        }

        // Guvenlik: Eger asiri kuculme sonucu ucgen kalmadiysa orijinal veriyi koru
        if (outIndices.empty())
        {
            outVertices = inVertices;
            outIndices = inIndices;
        }
    }

    bool mesh_collider_baker::bake(
        entity root,
        registry& reg,
        aria::system* ariaSys,
        mesh_collider_component& col
    )
    {
        std::vector<glm::vec3> rawVertices;
        std::vector<uint32_t> rawIndices;
        uint32_t gatheredSubmeshes = 0;

        const glm::mat4 rootWorld = ecs_transform::get_world_matrix(root, reg);

        // Kendi ve alt hiyerarsideki tum alt aglari topla
        collect_entity_meshes(
            root,
            rootWorld,
            reg,
            ariaSys,
            rawVertices,
            rawIndices,
            gatheredSubmeshes
        );

        if (rawVertices.empty() || rawIndices.empty())
        {
            col.submesh_count = 0;
            col.original_vertex_count = 0;
            col.original_triangle_count = 0;
            col.cooked_vertex_count = 0;
            col.cooked_triangle_count = 0;
            col.cooked_vertices.clear();
            col.cooked_indices.clear();
            col.is_dirty = false;
            return false;
        }

        // Center Offset uygula
        if (col.offset != glm::vec3(0.0f))
        {
            for (auto& v : rawVertices)
            {
                v += col.offset;
            }
        }

        col.submesh_count = gatheredSubmeshes;
        col.original_vertex_count = static_cast<uint32_t>(rawVertices.size());
        col.original_triangle_count = static_cast<uint32_t>(rawIndices.size() / 3);

        // Cozunurluk ayarina gore sadelestirme veya 1:1 birlestirme uygula
        if (col.resolution == mesh_collider_resolution::full)
        {
            weld_exact_vertices(rawVertices, rawIndices, col.cooked_vertices, col.cooked_indices);
        }
        else if (col.resolution == mesh_collider_resolution::high)
        {
            simplify_mesh(rawVertices, rawIndices, 85.0f, col.cooked_vertices, col.cooked_indices);
        }
        else if (col.resolution == mesh_collider_resolution::medium)
        {
            simplify_mesh(rawVertices, rawIndices, 42.0f, col.cooked_vertices, col.cooked_indices);
        }
        else if (col.resolution == mesh_collider_resolution::low)
        {
            simplify_mesh(rawVertices, rawIndices, 20.0f, col.cooked_vertices, col.cooked_indices);
        }
        else if (col.resolution == mesh_collider_resolution::convex_hull)
        {
            col.is_convex = true;

            // Jolt ConvexHullShape uzerinden gercek dis bukey kabuk yuzeylerini uret
            std::vector<glm::vec3> hullInputVerts;
            std::vector<uint32_t> hullInputIndices;
            if (rawVertices.size() > 2048)
            {
                simplify_mesh(rawVertices, rawIndices, 18.0f, hullInputVerts, hullInputIndices);
            }
            else
            {
                weld_exact_vertices(rawVertices, rawIndices, hullInputVerts, hullInputIndices);
            }

            JPH::Array<JPH::Vec3> points;
            points.reserve(hullInputVerts.size());
            for (const auto& v : hullInputVerts)
            {
                points.push_back(JPH::Vec3(v.x, v.y, v.z));
            }

            JPH::ConvexHullShapeSettings hullSettings(points);
            auto res = hullSettings.Create();
            if (res.IsValid())
            {
                const auto* hullShape = static_cast<const JPH::ConvexHullShape*>(res.Get().GetPtr());
                const JPH::Vec3 com = hullShape->GetCenterOfMass();
                const JPH::uint numPts = hullShape->GetNumPoints();

                col.cooked_vertices.clear();
                col.cooked_vertices.reserve(numPts);
                for (JPH::uint p = 0; p < numPts; ++p)
                {
                    const JPH::Vec3 pt = hullShape->GetPoint(p) + com;
                    col.cooked_vertices.push_back(glm::vec3(pt.GetX(), pt.GetY(), pt.GetZ()));
                }

                col.cooked_indices.clear();
                const JPH::uint numFaces = hullShape->GetNumFaces();
                std::vector<JPH::uint> faceVerts;
                for (JPH::uint f = 0; f < numFaces; ++f)
                {
                    const JPH::uint nv = hullShape->GetNumVerticesInFace(f);
                    if (nv < 3) continue;
                    faceVerts.resize(nv);
                    hullShape->GetFaceVertices(f, nv, faceVerts.data());
                    for (JPH::uint k = 1; k + 1 < nv; ++k)
                    {
                        col.cooked_indices.push_back(static_cast<uint32_t>(faceVerts[0]));
                        col.cooked_indices.push_back(static_cast<uint32_t>(faceVerts[k]));
                        col.cooked_indices.push_back(static_cast<uint32_t>(faceVerts[k + 1]));
                    }
                }
            }
            else
            {
                col.cooked_vertices = hullInputVerts;
                col.cooked_indices = hullInputIndices;
            }
        }

        col.cooked_vertex_count = static_cast<uint32_t>(col.cooked_vertices.size());
        col.cooked_triangle_count = static_cast<uint32_t>(col.cooked_indices.size() / 3);
        col.is_dirty = false;

        QLOG_INFO(physics, "[MeshColliderBaker] Varlik {} icin Mesh Collider basariyla pisirildi. Alt ag: {}, Orijinal: {} ucgen, Pisen: {} ucgen",
            root, col.submesh_count, col.original_triangle_count, col.cooked_triangle_count);

        return true;
    }

    JPH::ShapeRefC mesh_collider_baker::create_shape(
        const mesh_collider_component& col,
        const glm::vec3& worldScale
    )
    {
        const glm::vec3 effScale = glm::max(glm::abs(worldScale), glm::vec3(0.001f));

        if (col.cooked_vertices.empty())
        {
            // Geometri bulunamadiysa emniyetli kutu sekli dondur
            return new JPH::BoxShape(JPH::Vec3(effScale.x * 0.5f, effScale.y * 0.5f, effScale.z * 0.5f));
        }

        if (col.is_convex || col.resolution == mesh_collider_resolution::convex_hull)
        {
            JPH::Array<JPH::Vec3> points;
            const size_t maxHullInput = 2048;
            const size_t step = (col.cooked_vertices.size() > maxHullInput) ? (col.cooked_vertices.size() / maxHullInput) : 1;
            points.reserve(std::min(col.cooked_vertices.size(), maxHullInput));
            for (size_t i = 0; i < col.cooked_vertices.size(); i += step)
            {
                const auto& v = col.cooked_vertices[i];
                points.push_back(JPH::Vec3(v.x * effScale.x, v.y * effScale.y, v.z * effScale.z));
            }

            JPH::ConvexHullShapeSettings hullSettings(points);
            auto res = hullSettings.Create();
            if (res.IsValid())
            {
                return res.Get();
            }

            QLOG_WARN(physics, "[MeshColliderBaker] ConvexHullShape olusturulamadi, Box fallback kullaniliyor.");
            return new JPH::BoxShape(JPH::Vec3(effScale.x * 0.5f, effScale.y * 0.5f, effScale.z * 0.5f));
        }
        else
        {
            JPH::VertexList joltVertices;
            joltVertices.reserve(col.cooked_vertices.size());
            for (const auto& v : col.cooked_vertices)
            {
                joltVertices.push_back(JPH::Float3(v.x * effScale.x, v.y * effScale.y, v.z * effScale.z));
            }

            JPH::IndexedTriangleList joltTriangles;
            joltTriangles.reserve(col.cooked_indices.size() / 3);

            for (size_t i = 0; i + 2 < col.cooked_indices.size(); i += 3)
            {
                const uint32_t i0 = col.cooked_indices[i];
                const uint32_t i1 = col.cooked_indices[i + 1];
                const uint32_t i2 = col.cooked_indices[i + 2];
                if (i0 < joltVertices.size() && i1 < joltVertices.size() && i2 < joltVertices.size() &&
                    i0 != i1 && i1 != i2 && i0 != i2)
                {
                    joltTriangles.push_back(JPH::IndexedTriangle(i0, i1, i2, 0));
                }
            }

            if (!joltTriangles.empty())
            {
                JPH::MeshShapeSettings meshSettings(joltVertices, joltTriangles);
                meshSettings.Sanitize();
                auto res = meshSettings.Create();
                if (res.IsValid())
                {
                    return res.Get();
                }
                QLOG_WARN(physics, "[MeshColliderBaker] MeshShape olusturulamadi, Convex fallback deneniyor.");
            }

            // Fallback Convex Hull
            JPH::Array<JPH::Vec3> points;
            points.reserve(col.cooked_vertices.size());
            for (const auto& v : col.cooked_vertices)
            {
                points.push_back(JPH::Vec3(v.x * effScale.x, v.y * effScale.y, v.z * effScale.z));
            }
            JPH::ConvexHullShapeSettings hullSettings(points);
            auto res = hullSettings.Create();
            if (res.IsValid())
            {
                return res.Get();
            }

            return new JPH::BoxShape(JPH::Vec3(effScale.x * 0.5f, effScale.y * 0.5f, effScale.z * 0.5f));
        }
    }
} // namespace qual::physics
