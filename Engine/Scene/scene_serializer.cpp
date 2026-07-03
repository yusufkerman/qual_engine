#include "scene_serializer.h"
#include "scene.h"
#include "component_serialization.h"
#include "../Engine/engine.h" 
#include "../ECS/components.h" 
#include "../Importers/ModelLoader.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "../Engine/asset_pipeline.h"
#include "../Engine/Scripting/script_factory.h"

using json = nlohmann::json;

// 💡 NOT: Ham 'material' yapısı için yazılan manuel to_json ve from_json fonksiyonları 
// artık materyaller sahne dışına çıkarıldığı için bu dosyadan TEMİZLENDİ.

bool scene_serializer::serialize(const std::string& filepath, const scene& currentScene, EngineContext& ctx)
{
    const registry& reg = currentScene.get_registry();
    const auto& allEntities = reg.get_all_entities();

    json sceneJson;
    sceneJson["scene_name"] = "Aria_Engine_Scene";
    sceneJson["active_camera"] = currentScene.get_active_camera();

    // ============================================================================
    // 1. 🖼️ ASSET SERİLEŞTİRME (SADECE TEXTURES)
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

    // 🔥 ESKİ MATERYAL DÖNGÜLERİ VE HAM VERİ KAYDETME KISMI BAĞIMSIZ .JSON 
    // SİSTEMİNE GEÇİLDİĞİ İÇİN TAMAMEN KALDIRILDI.

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

        // 💡 Makro ve eklediğimiz şablon fonksiyonları sayesinde material_handle 
        // otomatik olarak indeks ve jenerasyon olarak hafifçe buraya basılacak.
        serialize_component_if_exists<material_component>(e, reg, entityJson, "material_component");

        serialize_component_if_exists<camera_component>(e, reg, entityJson, "camera_component");
        serialize_component_if_exists<light_component>(e, reg, entityJson, "light_component");
        serialize_component_if_exists<bounds_component>(e, reg, entityJson, "bounds_component");
        serialize_component_if_exists<rigidbody_component>(e, reg, entityJson, "rigidbody_component");
        serialize_component_if_exists<collider_component>(e, reg, entityJson, "collider_component");
        serialize_component_if_exists<native_script_component>(e, reg, entityJson, "native_script_component");

        sceneJson["entities"].push_back(entityJson);
    }

    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << sceneJson.dump(4);
    file.close();

    std::cout << "[Serializer] Sahne ve Aria Handle referanslari basariyla kaydedildi: " << filepath << std::endl;
    return true;
}

bool scene_serializer::deserialize(const std::string& filepath, scene& currentScene, EngineContext& ctx)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    json sceneJson;
    try { file >> sceneJson; }
    catch (const json::parse_error&) { file.close(); return false; }
    file.close();

    // Sahneyi ve sistemleri temizle
    currentScene.new_scene();
    ctx.aria_system->clear_all_resources();
    ctx.textureManager.clear();
    ctx.materialManager.clear();
    ctx.assetManager.clear();

    ctx.physicsSystem.shutdown();
    ctx.physicsSystem.init(ctx.session.editor_scene());

    std::unordered_map<unsigned int, entity> entityIdMap;
    registry& reg = currentScene.get_registry();

    // 1. ADIM: Entity'leri yarat
    if (sceneJson.contains("entities"))
    {
        for (const auto& entityJson : sceneJson["entities"])
        {
            entity newE = currentScene.create_entity();
            entityIdMap[entityJson["id"]] = newE;
        }

        // 2. ADIM: Komponentleri yükle
        for (const auto& entityJson : sceneJson["entities"])
        {
            entity newE = entityIdMap[entityJson["id"]];

            deserialize_component_if_exists<name_component>(newE, reg, entityJson, "name_component");
            deserialize_component_if_exists<transform_component>(newE, reg, entityJson, "transform_component");
            deserialize_component_if_exists<camera_component>(newE, reg, entityJson, "camera_component");
            deserialize_component_if_exists<light_component>(newE, reg, entityJson, "light_component");
            deserialize_component_if_exists<bounds_component>(newE, reg, entityJson, "bounds_component");
            deserialize_component_if_exists<rigidbody_component>(newE, reg, entityJson, "rigidbody_component");
            deserialize_component_if_exists<collider_component>(newE, reg, entityJson, "collider_component");

            // Native Script
            if (entityJson.contains("native_script_component"))
            {
                std::string s_name = entityJson["native_script_component"].value("script_name", "None");
                if (s_name != "None") script_factory::instantiate_script(s_name, newE, reg);
            }

            // 🔥 YENİ SİSTEM: Mesh Component (Artık sadece handle)
            if (entityJson.contains("mesh_component"))
            {
                mesh_component meshComp = entityJson["mesh_component"].get<mesh_component>();
                reg.add_component<mesh_component>(newE, meshComp);
            }

            // 🔥 YENİ SİSTEM: Material Component
            if (entityJson.contains("material_component"))
            {
                material_component matComp = entityJson["material_component"].get<material_component>();
                reg.add_component<material_component>(newE, matComp);
            }
        }
    }

    // Kamera ve Işık güncellemeleri
    if (sceneJson.contains("active_camera"))
    {
        unsigned int oldCamId = sceneJson["active_camera"];
        if (entityIdMap.count(oldCamId)) currentScene.set_active_camera(entityIdMap[oldCamId]);
    }

    currentScene.update_light_batch();
    return true;
}