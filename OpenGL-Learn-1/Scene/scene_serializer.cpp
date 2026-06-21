#include "scene_serializer.h"
#include "scene.h"
#include "../Renderer/material_manager.h" 
#include "component_serialization.h"
#include "../Engine/engine.h" 
#include "../ECS/components.h" 
#include "../Importers/ModelLoader.h" // Model yükleyici için
#include <fstream>
#include <iostream>
#include <unordered_set>

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

        sceneJson["entities"].push_back(entityJson);
    }

    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << sceneJson.dump(4);
    file.close();

    std::cout << "[Serializer] Sahne ve tüm Asset verileri basariyla kaydedildi: " << filepath << std::endl;
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
    ctx.textureManager.clear();      // Tüm Dokular silindi

    // ============================================================================
    // 1. 🗺️ ID TERCÜME TABLOLARI (CRASH ÖNLEYİCİLER)
    // ============================================================================
    std::unordered_map<unsigned int, unsigned int> textureIdMap;
    std::unordered_map<unsigned int, unsigned int> materialIdMap;
    std::unordered_map<unsigned int, unsigned int> meshIdMap; // Eski Mesh ID -> Yeni Mesh ID (Yeni!)

    textureIdMap[0] = 0;
    materialIdMap[0] = 0;
    meshIdMap[0] = 0;

    // 2. 🖼️ DOKULARI DISKTEN YENİDEN YÜKLE
    if (sceneJson.contains("textures") && sceneJson["textures"].is_array())
    {
        for (const auto& texJson : sceneJson["textures"])
        {
            unsigned int oldTexId = texJson["id"];
            std::string texPath = texJson["path"];

            unsigned int newTexId = ctx.textureManager.load(texPath);

            // 🚨 DOSYA SİLİNMİŞ/DEĞİŞTİRİLMİŞ KONTROLÜ:
            // Eğer texture manager 0 döndüyse yükleme başarısız olmuştur.
            if (newTexId == 0)
            {
                std::cout << "[Serializer KORUMA] Doku dosyasi bulunamadi: "
                    << texPath << " -> Pas geciliyor, varsayilan 0 (No Texture) atanacak." << std::endl;

                textureIdMap[oldTexId] = 0; // 0 standardı (Doku yok/Beyaz doku)
            }
            else
            {
                textureIdMap[oldTexId] = newTexId;
            }
        }
    }

    // 3. 🎨 MATERYALLERİ YENİDEN İNŞA ET
    if (sceneJson.contains("materials") && sceneJson["materials"].is_array())
    {
        for (const auto& matJson : sceneJson["materials"])
        {
            unsigned int oldMatId = matJson["id"];
            material matData = matJson["data"].get<material>();

            if (textureIdMap.find(matData.diffuse_texture) != textureIdMap.end())
                matData.diffuse_texture = textureIdMap[matData.diffuse_texture];
            if (textureIdMap.find(matData.normal_texture) != textureIdMap.end())
                matData.normal_texture = textureIdMap[matData.normal_texture];
            if (textureIdMap.find(matData.specular_texture) != textureIdMap.end())
                matData.specular_texture = textureIdMap[matData.specular_texture];

            unsigned int newMatId = ctx.materialManager.create(matData);
            materialIdMap[oldMatId] = newMatId;
        }
    }

    // ============================================================================
    // 4. 📐 MODELLERİ (MESH) DISKTEN ÖNDEN YÜKLEME VE TERCÜME ETME (Yeni!)
    // ============================================================================
    // Mükemmel hile: Entity'leri yüklemeden önce tüm mesh_component'leri tarayıp 
    // hangi modellerin yüklenmesi gerektiğini buluyoruz ve çökmenin önüne geçiyoruz.
    if (sceneJson.contains("entities") && sceneJson["entities"].is_array())
    {
        for (const auto& entityJson : sceneJson["entities"])
        {
            if (entityJson.contains("mesh_component"))
            {
                mesh_component meshComp = entityJson["mesh_component"].get<mesh_component>();

                if (meshComp.model_path != "None" && meshIdMap.find(meshComp.mesh_id) == meshIdMap.end())
                {
                    std::cout << "[Serializer] Model yukleniyor: " << meshComp.model_path << std::endl;
                    Model loadedModel = ModelLoader::Load(meshComp.model_path);

                    // 🚨 DOSYA SİLİNMİŞ/DEĞİŞTİRİLMİŞ KONTROLÜ:
                    // Eğer model yüklenemediyse (içinde mesh yoksa), sistemi çökertme, pas geç!
                    if (loadedModel.meshes.empty())
                    {
                        std::cout << "[Serializer KORUMA] Model dosyasi bulunamadi veya hatali: "
                            << meshComp.model_path << " -> Pas geciliyor!" << std::endl;

                        // Bu eski ID'yi haritada 0'a (veya varsa varsayılan bir error_cube mesh'ine) eşle
                        meshIdMap[meshComp.mesh_id] = 0;
                        continue;
                    }

                    int newMeshId = ctx.renderer.UploadModel(loadedModel);
                    meshIdMap[meshComp.mesh_id] = newMeshId;
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
        for (const auto& entityJson : sceneJson["entities"])
        {
            if (!entityJson.contains("id")) continue;

            entity newE = currentScene.create_entity();

            // Standart bileşenleri düz yükle
            deserialize_component_if_exists<name_component>(newE, reg, entityJson, "name_component");
            deserialize_component_if_exists<transform_component>(newE, reg, entityJson, "transform_component");
            deserialize_component_if_exists<camera_component>(newE, reg, entityJson, "camera_component");
            deserialize_component_if_exists<light_component>(newE, reg, entityJson, "light_component");
            deserialize_component_if_exists<bounds_component>(newE, reg, entityJson, "bounds_component");

            // 📐 Mesh Component ID Güncellemesi (Tercüme)
            if (entityJson.contains("mesh_component"))
            {
                mesh_component meshComp = entityJson["mesh_component"].get<mesh_component>();
                if (meshIdMap.find(meshComp.mesh_id) != meshIdMap.end())
                {
                    meshComp.mesh_id = meshIdMap[meshComp.mesh_id];
                }
                reg.add_component<mesh_component>(newE, meshComp);
            }

            // 🎨 Material Component ID Güncellemesi (Tercüme)
            if (entityJson.contains("material_component"))
            {
                material_component matComp = entityJson["material_component"].get<material_component>();
                if (materialIdMap.find(matComp.material_id) != materialIdMap.end())
                {
                    matComp.material_id = materialIdMap[matComp.material_id];
                }
                reg.add_component<material_component>(newE, matComp);
            }
        }
    }

    // 6. 🎥 AKTİF KAMERAYI SET ET
    if (sceneJson.contains("active_camera"))
    {
        entity savedCam = sceneJson["active_camera"];
        if (savedCam != 0) currentScene.set_active_camera(savedCam);
    }

    currentScene.update_light_batch();

    std::cout << "[Serializer] Sahne, Modeller, Materyaller ve Dokular KURŞUN GEÇİRMEZ şekilde yüklendi!" << std::endl;
    return true;
}