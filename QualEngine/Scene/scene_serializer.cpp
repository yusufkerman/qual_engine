#include "scene_serializer.h"
#include "scene.h"
#include "../Renderer/material_manager.h" 
#include "component_serialization.h"
#include "../Engine/engine.h" 
#include "../ECS/components.h" 
#include "../Importers/ModelLoader.h" // Model yükleyici için
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "../Engine/asset_pipeline.h"

using json = nlohmann::json;

bool scene_serializer::serialize(const std::string& filepath, const scene& currentScene, EngineContext& ctx)
{
    const registry& reg = currentScene.get_registry();
    const auto& allEntities = reg.get_all_entities();

    json sceneJson;
    sceneJson["scene_name"] = "PS1_Engine_Scene";
    sceneJson["active_camera"] = currentScene.get_active_camera();

    // ============================================================================
    // 1. 🖼️ ASSET SERİLEŞTİRME (TEXTURES & MATERIALS)
    // ============================================================================
    sceneJson["textures"] = json::array();
    const auto& allTexturePaths = ctx.textureManager.get_all_paths();
    for (const auto& [texId, path] : allTexturePaths)
    {
        json texJson;
        texJson["id"] = texId;
        texJson["path"] = path;
        sceneJson["textures"].push_back(texJson);
    }

    sceneJson["materials"] = json::array();
    const auto& allMaterials = ctx.materialManager.get_all_materials();
    for (const auto& [matId, matData] : allMaterials)
    {
        json matJson;
        matJson["id"] = matId;
        matJson["data"] = matData;
        sceneJson["materials"].push_back(matJson);
    }

    // ============================================================================
    // 2. 👾 ENTITY & COMPONENT SERİLEŞTİRME
    // ============================================================================
    sceneJson["entities"] = json::array();
    for (entity e : allEntities)
    {
        json entityJson;
        entityJson["id"] = e;

        serialize_component_if_exists<name_component>(e, reg, entityJson, "name_component");
        serialize_component_if_exists<transform_component>(e, reg, entityJson, "transform_component");
        serialize_component_if_exists<mesh_component>(e, reg, entityJson, "mesh_component");
        serialize_component_if_exists<material_component>(e, reg, entityJson, "material_component");
        serialize_component_if_exists<camera_component>(e, reg, entityJson, "camera_component");
        serialize_component_if_exists<light_component>(e, reg, entityJson, "light_component");
        serialize_component_if_exists<bounds_component>(e, reg, entityJson, "bounds_component");

        // Fizik Bileşenleri
        serialize_component_if_exists<rigidbody_component>(e, reg, entityJson, "rigidbody_component");
        serialize_component_if_exists<collider_component>(e, reg, entityJson, "collider_component");

        sceneJson["entities"].push_back(entityJson);
    }

    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << sceneJson.dump(4);
    file.close();

    std::cout << "[Serializer] Sahne ve tum Asset verileri basariyla kaydedildi: " << filepath << std::endl;
    return true;
}

bool scene_serializer::deserialize(const std::string& filepath, scene& currentScene, EngineContext& ctx)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cout << "[Serializer] Sahne dosyasi acilamadi: " << filepath << std::endl;
        return false;
    }

    json sceneJson;
    try { file >> sceneJson; }
    catch (const json::parse_error& e) {
        std::cout << "[Serializer] JSON Ayristirma hatasi: " << e.what() << std::endl;
        file.close();
        return false;
    }
    file.close();

    // 🚨 ESKİ SAHNEYİ TEMİZLE
    currentScene.new_scene();

    ctx.renderer.Clear();            // Tüm VAO/VBO'lar silindi
    asset_pipeline::clear_cache();
    ctx.textureManager.clear();      // Tüm Dokular silindi

    //Fizik temizliği
    ctx.physicsSystem.shutdown();
    if (!ctx.physicsSystem.init())
    {
        std::cout << "[Jolt Error] Sahne yuklenirken fizik sistemi yeniden baslatilamadi!" << std::endl;
        return false;
    }

    // ============================================================================
    // 1. 🗺️ ID TERCÜME TABLOLARI (CRASH ÖNLEYİCİLER)
    // ============================================================================
    std::unordered_map<unsigned int, unsigned int> textureIdMap;
    std::unordered_map<unsigned int, unsigned int> materialIdMap;
    std::unordered_map<unsigned int, unsigned int> meshIdMap;
    std::unordered_map<unsigned int, unsigned int> entityIdMap; // 🔥 KRİTİK: Eski Entity ID -> Yeni Entity ID haritası

    textureIdMap[0] = 0;
    materialIdMap[0] = 0;
    //meshIdMap[0] = 0;
    entityIdMap[0] = 0;

    // 2. 🖼️ DOKULARI DISKTEN YENİDEN YÜKLE
    if (sceneJson.contains("textures") && sceneJson["textures"].is_array())
    {
        for (const auto& texJson : sceneJson["textures"])
        {
            if (!texJson.contains("id") || !texJson.contains("path")) continue;

            unsigned int oldTexId = texJson["id"];
            std::string texPath = texJson["path"];

            unsigned int newTexId = ctx.textureManager.load(texPath);
            textureIdMap[oldTexId] = (newTexId == 0) ? 0 : newTexId;
        }
    }

    // 3. 🎨 MATERYALLERİ YENİDEN İNŞA ET
    if (sceneJson.contains("materials") && sceneJson["materials"].is_array())
    {
        for (const auto& matJson : sceneJson["materials"])
        {
            if (!matJson.contains("id") || !matJson.contains("data")) continue;

            unsigned int oldMatId = matJson["id"];
            material matData = matJson["data"].get<material>();

            auto itDiff = textureIdMap.find(matData.diffuse_texture);
            matData.diffuse_texture = (itDiff != textureIdMap.end()) ? itDiff->second : 0;

            auto itNorm = textureIdMap.find(matData.normal_texture);
            matData.normal_texture = (itNorm != textureIdMap.end()) ? itNorm->second : 0;

            auto itSpec = textureIdMap.find(matData.specular_texture);
            matData.specular_texture = (itSpec != textureIdMap.end()) ? itSpec->second : 0;

            unsigned int newMatId = ctx.materialManager.create(matData);
            materialIdMap[oldMatId] = newMatId;
        }
    }

    // ============================================================================
    // 4. 📐 MODELLERİ (MESH) DISKTEN ÖNDEN YÜKLEME VE TERCÜME ETME
    // ============================================================================
    if (sceneJson.contains("entities") && sceneJson["entities"].is_array())
    {
        for (const auto& entityJson : sceneJson["entities"])
        {
            if (entityJson.contains("mesh_component") && !entityJson["mesh_component"].is_null())
            {
                mesh_component meshComp = entityJson["mesh_component"].get<mesh_component>();

                if (meshComp.model_path != "None" && meshIdMap.find(meshComp.mesh_id) == meshIdMap.end())
                {
                    int newMeshId = 0;

                    if (meshComp.model_path.rfind("internal://primitive_cube", 0) == 0)
                    {
                        auto result = asset_pipeline::create_cube_mesh(ctx);
                        newMeshId = result.mesh_ids.empty() ? 0 : result.mesh_ids[0];
                    }
                    else if (meshComp.model_path.rfind("internal://primitive_sphere", 0) == 0)
                    {
                        // path'ten radius'u parse et
                        float radius = 1.0f;
                        auto pos = meshComp.model_path.rfind('_');
                        if (pos != std::string::npos)
                            radius = std::stof(meshComp.model_path.substr(pos + 1));

                        auto result = asset_pipeline::create_sphere_mesh(ctx, radius);
                        newMeshId = result.mesh_ids.empty() ? 0 : result.mesh_ids[0];
                    }
                    else
                    {
                        Model loadedModel = ModelLoader::Load(meshComp.model_path);
                        if (!loadedModel.meshes.empty())
                            newMeshId = ctx.renderer.UploadModel(loadedModel);
                    }

                    meshIdMap[meshComp.mesh_id] = newMeshId;

                    std::cout << "[DEBUG] model_path: " << meshComp.model_path
                        << " | old mesh_id: " << meshComp.mesh_id
                        << " | new mesh_id: " << newMeshId << std::endl;
                }
            }
        }
    }

    // ============================================================================
    // 5. 👾 ENTITY VE BİLEŞENLERİ SAHNEYE DÖŞEME
    // ============================================================================
    registry& reg = currentScene.get_registry();

    if (sceneJson.contains("entities") && sceneJson["entities"].is_array())
    {
        // 🌟 İLK TUR: Önce tüm boş entity'leri oluştur ve ID eşleşmesini kaydet!
        for (const auto& entityJson : sceneJson["entities"])
        {
            if (!entityJson.contains("id")) continue;

            unsigned int oldEntityId = entityJson["id"];
            entity newE = currentScene.create_entity();

            entityIdMap[oldEntityId] = newE; // Eski ID'yi yeni ID'ye bağladık!
        }

        // 🌟 İKİNCİ TUR: Şimdi bileşenleri güvenle üzerlerine yerleştir
        int entityIndex = 0;
        for (const auto& entityJson : sceneJson["entities"])
        {
            if (!entityJson.contains("id")) continue;

            unsigned int oldEntityId = entityJson["id"];
            entity newE = entityIdMap[oldEntityId]; // Yeni oluşturduğumuz geçerli ID'yi aldık

            // Bileşenleri yükle
            deserialize_component_if_exists<name_component>(newE, reg, entityJson, "name_component");
            deserialize_component_if_exists<transform_component>(newE, reg, entityJson, "transform_component");
            deserialize_component_if_exists<camera_component>(newE, reg, entityJson, "camera_component");
            deserialize_component_if_exists<light_component>(newE, reg, entityJson, "light_component");
            deserialize_component_if_exists<bounds_component>(newE, reg, entityJson, "bounds_component");
            deserialize_component_if_exists<rigidbody_component>(newE, reg, entityJson, "rigidbody_component");
            deserialize_component_if_exists<collider_component>(newE, reg, entityJson, "collider_component");

            // Mesh ID Güncellemesi
            if (entityJson.contains("mesh_component") && !entityJson["mesh_component"].is_null())
            {
                mesh_component meshComp = entityJson["mesh_component"].get<mesh_component>();
                auto itMesh = meshIdMap.find(meshComp.mesh_id);
                meshComp.mesh_id = (itMesh != meshIdMap.end()) ? itMesh->second : 0;

                std::cout << "[DEBUG] Entity mesh_id set to: " << meshComp.mesh_id << std::endl;

                reg.add_component<mesh_component>(newE, meshComp);
            }

            // Material ID Güncellemesi
            if (entityJson.contains("material_component") && !entityJson["material_component"].is_null())
            {
                material_component matComp = entityJson["material_component"].get<material_component>();
                auto itMat = materialIdMap.find(matComp.material_id);
                matComp.material_id = (itMat != materialIdMap.end()) ? itMat->second : 0;
                reg.add_component<material_component>(newE, matComp);
            }
        }
    }

    // 6. 🎥 AKTİF KAMERAYI SET ET (GÜVENLİ TERCÜME)
    if (sceneJson.contains("active_camera"))
    {
        unsigned int oldCamId = sceneJson["active_camera"];

        // 🌟 Eğer eski kamera ID'si haritamızda varsa yeni karşılığını ata, yoksa atlama yapma!
        if (entityIdMap.find(oldCamId) != entityIdMap.end())
        {
            entity newCamId = entityIdMap[oldCamId];
            if (newCamId != 0) {
                currentScene.set_active_camera(newCamId);
            }
        }
    }

    currentScene.update_light_batch();

    std::cout << "[Serializer] Sahne tamamen basariyla yuklendi!" << std::endl;
    return true;
}