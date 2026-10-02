/**
 * @file scene_serializer.cpp
 * @brief Sahne serileştirme ve JSON varlık çözümleme uygulaması.
 */

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

bool scene_serializer::serialize(const std::string& filepath, const scene& currentScene, EngineContext& ctx)
{
    const registry& reg = currentScene.get_registry();
    const auto& allEntities = reg.get_all_entities();

    json sceneJson;
    sceneJson["scene_name"] = "Aria_Engine_Scene";
    sceneJson["active_camera"] = currentScene.get_active_camera();

    // 1. Doku referanslarının serileştirilmesi (Legacy doku kayıt defteri)
    sceneJson["textures"] = json::array();
    const auto& allTexturePaths = ctx.textureManager.get_all_paths();
    for (const auto& [texId, path] : allTexturePaths)
    {
        json texJson;
        texJson["id"] = texId;
        texJson["path"] = path;
        sceneJson["textures"].push_back(texJson);
    }

    // 2. Varlık ve bileşen verilerinin serileştirilmesi
    sceneJson["entities"] = json::array();
    for (const entity e : allEntities)
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
        serialize_component_if_exists<rigidbody_component>(e, reg, entityJson, "rigidbody_component");
        serialize_component_if_exists<collider_component>(e, reg, entityJson, "collider_component");
        serialize_component_if_exists<native_script_component>(e, reg, entityJson, "native_script_component");

        sceneJson["entities"].push_back(entityJson);
    }

    std::ofstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[Serializer] Dosya acilamadi: " << filepath << '\n';
        return false;
    }

    file << sceneJson.dump(4);
    file.close();

    std::cout << "[Serializer] Sahne basariyla kaydedildi: " << filepath << '\n';
    return true;
}

bool scene_serializer::deserialize(const std::string& filepath, scene& currentScene, EngineContext& ctx)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[Serializer] Dosya bulunamadi: " << filepath << '\n';
        return false;
    }

    json sceneJson;
    try
    {
        file >> sceneJson;
    }
    catch (const json::parse_error& e)
    {
        std::cerr << "[Serializer] JSON parse hatasi: " << e.what() << '\n';
        file.close();
        return false;
    }
    file.close();

    // Sahneyi ve sistem önbelleklerini sıfırla
    currentScene.new_scene();

    if (ctx.aria_system)
    {
        ctx.aria_system->clear_all_resources();
    }

    ctx.textureManager.clear();
    ctx.materialManager.clear();
    ctx.assetManager.clear();

    ctx.physicsSystem.shutdown();
    ctx.physicsSystem.init(ctx.session.editor_scene());

    std::unordered_map<unsigned int, entity> entityIdMap;
    registry& reg = currentScene.get_registry();

    // 1. Aşama: Varlık tanıtıcılarını oluştur
    if (sceneJson.contains("entities"))
    {
        for (const auto& entityJson : sceneJson["entities"])
        {
            const entity newE = currentScene.create_entity();
            entityIdMap[entityJson["id"].get<unsigned int>()] = newE;
        }

        // 2. Aşama: Bileşenleri yeni varlıklara bağla
        for (const auto& entityJson : sceneJson["entities"])
        {
            const entity newE = entityIdMap[entityJson["id"].get<unsigned int>()];

            deserialize_component_if_exists<name_component>(newE, reg, entityJson, "name_component");
            deserialize_component_if_exists<transform_component>(newE, reg, entityJson, "transform_component");
            deserialize_component_if_exists<camera_component>(newE, reg, entityJson, "camera_component");
            deserialize_component_if_exists<light_component>(newE, reg, entityJson, "light_component");
            deserialize_component_if_exists<bounds_component>(newE, reg, entityJson, "bounds_component");
            deserialize_component_if_exists<rigidbody_component>(newE, reg, entityJson, "rigidbody_component");
            deserialize_component_if_exists<collider_component>(newE, reg, entityJson, "collider_component");

            if (entityJson.contains("native_script_component"))
            {
                const std::string s_name = entityJson["native_script_component"].value("script_name", "None");
                if (s_name != "None")
                {
                    script_factory::instantiate_script(s_name, newE, reg);
                }
            }

            if (entityJson.contains("mesh_component"))
            {
                mesh_component meshComp = entityJson["mesh_component"].get<mesh_component>();
                reg.add_component<mesh_component>(newE, meshComp);
            }

            if (entityJson.contains("material_component"))
            {
                material_component matComp = entityJson["material_component"].get<material_component>();
                reg.add_component<material_component>(newE, matComp);
            }
        }
    }

    // Aktif kamera ve ışık durumunu senkronize et
    if (sceneJson.contains("active_camera"))
    {
        const unsigned int oldCamId = sceneJson["active_camera"].get<unsigned int>();
        const auto it = entityIdMap.find(oldCamId);
        if (it != entityIdMap.end())
        {
            currentScene.set_active_camera(it->second);
        }
    }

    currentScene.update_light_batch();
    return true;
}