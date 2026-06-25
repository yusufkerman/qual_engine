// Dosya Yolu: sandbox.cpp
#include "sandbox.h"
#include "Engine/engine_context.h"
#include "Engine/asset_pipeline.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <Scene/scene_serializer.h>

void sandbox_load_default_scene(EngineContext& ctx)
{
    scene& editorScene = ctx.session.editor_scene();
    ctx.editor.setScene(&editorScene);

    // =========================================================
    // MATERIAL (Sadece renk amaçlı basit bir materyal)
    // =========================================================
    material mat;
    mat.shader_id = 0;
    mat.diffuse_texture = 0; // Herhangi bir doku yüklemiyoruz, default beyaz kalabilir
    mat.normal_texture = 0;
    mat.normal_strength = 1.0f;

    unsigned int materialID = ctx.materialManager.create(mat);
    std::cout << "material created, id: " << materialID << std::endl;
    std::cout << "materialManager size: " << ctx.materialManager.get_all_materials().size() << std::endl;


    // =========================================================
    // 1. PRIMITIVE: STATIC FLOOR (Küpü Plane gibi kullanıyoruz)
    // =========================================================
    auto cubeImport = asset_pipeline::create_cube_mesh(ctx);
    entity floorEntity = editorScene.create_entity();

    editorScene.get_registry().add_component<name_component>(
        floorEntity,
        name_component{ "Static Floor (Plane)" }
    );

    editorScene.get_registry().add_component<mesh_component>(
        floorEntity,
        mesh_component{ cubeImport.mesh_ids.front(), "internal://primitive_cube" }
    );

    editorScene.get_registry().add_component<material_component>(
        floorEntity,
        material_component{ materialID }
    );

    editorScene.get_registry().add_component<bounds_component>(
        floorEntity,
        bounds_component{ cubeImport.boundsCenter, cubeImport.boundsRadius }
    );

    // Küpü ince ve çok geniş yaparak bir zemin (Plane) haline getiriyoruz
    editorScene.get_registry().add_component<transform_component>(
        floorEntity,
        transform_component{
            glm::vec3(0.0f, -20.0f, 0.0f), // Aşağıda konumlandı
            glm::vec3(0.0f),
            glm::vec3(100.0f, 1.0f, 100.0f) // Geniş zemin ölçeği
        }
    );

    // Jolt Fizik: Küpün scale değerinin yarısını extents olarak veriyoruz (50x0.5x50)
    editorScene.get_registry().add_component<collider_component>(
        floorEntity,
        collider_component{ collider_type::box, glm::vec3(50.0f, 0.5f, 50.0f) }
    );

    editorScene.get_registry().add_component<rigidbody_component>(
        floorEntity,
        rigidbody_component{ motion_type::kinematic_body, 0.0f, 0.6f, 0.0f } // Statik, kımıldamaz
    );

    // =========================================================
    // 2. PRIMITIVE: DYNAMIC SPHERE (Yukarıdan düşecek olan top)
    // =========================================================
    float sphereRadius = 2.0f;
    auto sphereImport = asset_pipeline::create_sphere_mesh(ctx, sphereRadius);
    entity sphereEntity = editorScene.create_entity();

    editorScene.get_registry().add_component<name_component>(
        sphereEntity,
        name_component{ "Falling Sphere" }
    );

    editorScene.get_registry().add_component<mesh_component>(
        sphereEntity,
        mesh_component{ sphereImport.mesh_ids.front(), "internal://primitive_sphere_" + std::to_string(sphereRadius) }
    );

    editorScene.get_registry().add_component<material_component>(
        sphereEntity,
        material_component{ materialID }
    );

    editorScene.get_registry().add_component<bounds_component>(
        sphereEntity,
        bounds_component{ sphereImport.boundsCenter, sphereImport.boundsRadius }
    );

    // Küreyi zeminin yukarısında havada başlatıyoruz
    editorScene.get_registry().add_component<transform_component>(
        sphereEntity,
        transform_component{
            glm::vec3(0.0f, 15.0f, 0.0f), // Havada başladı
            glm::vec3(0.0f),
            glm::vec3(1.0f)
        }
    );

    // Jolt Fizik: Küre collider yarıçapını ayarlıyoruz
    editorScene.get_registry().add_component<collider_component>(
        sphereEntity,
        collider_component{ collider_type::sphere, glm::vec3(sphereRadius) }
    );

    // Dinamik nesne: 5kg ağırlık ve zıplama (restitution) 0.5f verilerek sekmesi sağlandı
    editorScene.get_registry().add_component<rigidbody_component>(
        sphereEntity,
        rigidbody_component{ motion_type::dynamic_body, 5.0f, 0.4f, 0.5f }
    );

    // =========================================================
    // CAMERA (Aynen Korundu)
    // =========================================================
    entity gameCameraEntity = editorScene.create_entity();

    editorScene.get_registry().add_component<transform_component>(
        gameCameraEntity,
        transform_component{
            glm::vec3(0.0f, -5.0f, 70.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f)
        }
    );

    editorScene.get_registry().add_component<camera_component>(
        gameCameraEntity,
        camera_component{
            45.0f,
            0.1f,
            1000.0f,
            true
        }
    );

    editorScene.get_registry().add_component<name_component>(
        gameCameraEntity,
        name_component{ "Game Camera" }
    );

    // =========================================================
    // LIGHT (Aynen Korundu)
    // =========================================================
    entity lightEntity = editorScene.create_entity();

    editorScene.get_registry().add_component<transform_component>(
        lightEntity,
        transform_component{
            glm::vec3(25.0f, 110.0f, 5.0f),
            glm::vec3(1.0f),
            glm::vec3(1.0f)
        }
    );

    light_component pointLight;
    pointLight.type = light_type::spot;
    pointLight.color = glm::vec3(1.0f, 0.95f, 0.8f);
    pointLight.intensity = 3.0f;
    pointLight.range = 150.0f;
    pointLight.inner_cone = 2.0f;
    pointLight.outer_cone = 10.0f;

    editorScene.get_registry().add_component<light_component>(
        lightEntity,
        pointLight
    );

    editorScene.get_registry().add_component<name_component>(
        lightEntity,
        name_component{ "Test Point Light" }
    );
}
void sandbox_load_scene2_directly(EngineContext& ctx)
{
    //  ÖNEMLİ DEĞİŞİKLİK: 
    // Eğer isEditorBuild = false ise motor çizim için 'game_scene()' referansını kullanır.
    // Doğru sahne referansını seçmezsek yüklenen veriler boşa gider ve ekran mor kalır.
    scene& activeScene = ctx.session.editor_scene();

    std::string scenePath = "Assets/scene2.json";
    std::cout << "[Sandbox] '" << scenePath << "' dosyasi serializer uzerinden dogrudan yukleniyor..." << std::endl;

    // Sahneyi o an aktif olan sahne yapısına yüklüyoruz (Tam olarak UI'ın yaptığı gibi)
    bool success = scene_serializer::deserialize(scenePath, activeScene, ctx);

    if (!success)
    {
        std::cout << "[Sandbox] DOĞRUDAN YÜKLEME BAŞARISIZ! Default sahneye donulüyor..." << std::endl;
        sandbox_load_default_scene(ctx);
        return;
    }

    // 🔥 Standalone moddaysak editörün de ne çizdiğini bilmesi için kopyalama yapıyoruz
    if (!ctx.isEditorBuild)
    {
        ctx.session.editor_scene() = ctx.session.game_scene();
    }

    // Editörün takip ettiği sahne işaretçisini güncelliyoruz
    ctx.editor.setScene(&activeScene);
}