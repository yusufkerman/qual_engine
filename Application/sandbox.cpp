// Dosya Yolu: sandbox.cpp
#include "sandbox.h"
#include "Engine/engine_context.h"
#include "Engine/asset_pipeline.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <Scene/scene_serializer.h>

#include "Assets/Scripts/player_controller.h"
#include "Assets/Scripts/camera_controller.h" 

void sandbox_load_default_scene(EngineContext& ctx)
{
    
}

void sandbox_load_scene2_directly(EngineContext& ctx)
{
    
}

// =========================================================
// 🎮 NEW FUNCTION: SANDBOX DEMO GAME
// =========================================================
void sandbox_load_demo_game(EngineContext& ctx)
{
    scene& editorScene = ctx.session.editor_scene();
    ctx.editor.setScene(&editorScene);

    // ---------------------------------------------------------
    // 🛠️ MATERYAL DÜZELTMESİ (ARIA HANDLE ENTEGRASYONU)
    // ---------------------------------------------------------
    // Eski materialManager'dan üretilen ID'yi component kabul etmediği için,
    // doğrudan varsayılan, temiz bir Aria Handle oluşturuyoruz.
    aria::handle<aria::material_asset> matHandle{};

    // 🚀 YENİ ARIA API: Cube handle üretiliyor
    aria::handle<aria::model_asset> cubeHandle = asset_pipeline::create_cube_mesh(ctx);

    // ---------------------------------------------------------
    // 1. STATIC FLOOR
    // ---------------------------------------------------------
    entity floorEntity = editorScene.create_entity();
    editorScene.get_registry().add_component<name_component>(floorEntity, name_component{ "Zemin (Static)" });
    editorScene.get_registry().add_component<mesh_component>(floorEntity, mesh_component{ cubeHandle });

    // 🚀 Artık hata vermeyen Aria tabanlı matHandle paslanıyor
    editorScene.get_registry().add_component<material_component>(floorEntity, material_component{ matHandle });
    editorScene.get_registry().add_component<bounds_component>(floorEntity, bounds_component{ glm::vec3(0.0f), 1.0f });

    editorScene.get_registry().add_component<transform_component>(
        floorEntity,
        transform_component{
            glm::vec3(0.0f, -10.0f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(150.0f, 1.0f, 150.0f)
        }
    );
    editorScene.get_registry().add_component<collider_component>(floorEntity, collider_component{ collider_type::box, glm::vec3(75.0f, 0.5f, 75.0f) });
    editorScene.get_registry().add_component<rigidbody_component>(floorEntity, rigidbody_component{ motion_type::kinematic_body, 0.0f, 0.6f, 0.0f });

    // ---------------------------------------------------------
    // 2. PLAYER
    // ---------------------------------------------------------
    entity playerEntity = editorScene.create_entity();
    editorScene.get_registry().add_component<name_component>(playerEntity, name_component{ "Player" });

    editorScene.get_registry().add_component<transform_component>(
        playerEntity,
        transform_component{
            glm::vec3(0.0f, -5.0f, 0.0f),
            glm::vec3(0.0f),
            glm::vec3(2.0f, 5.0f, 2.0f)
        }
    );

    editorScene.get_registry().add_component<mesh_component>(
        playerEntity,
        mesh_component{ cubeHandle }
    );

    // 🚀 Aynı şekilde oyuncu için de Aria tabanlı matHandle paslanıyor
    editorScene.get_registry().add_component<material_component>(
        playerEntity,
        material_component{ matHandle });

    editorScene.get_registry().add_component<bounds_component>(
        playerEntity,
        bounds_component{ glm::vec3(0.0f), 1.0f }
    );

    editorScene.get_registry().add_component<collider_component>(
        playerEntity,
        collider_component{ collider_type::box, glm::vec3(1.0f, 2.5f, 1.0f) }
    );

    rigidbody_component playerRB;
    playerRB.motion = motion_type::dynamic_body;
    playerRB.mass = 70.0f;
    playerRB.friction = 0.3f;
    playerRB.restitution = 0.0f;
    playerRB.freeze_rotation_x = true;
    playerRB.freeze_rotation_z = true;
    playerRB.freeze_rotation_y = false;

    editorScene.get_registry().add_component<rigidbody_component>(playerEntity, playerRB);

    auto& scriptComp = editorScene.get_registry().add_component<native_script_component>(playerEntity);
    //scriptComp.bind<player_controller>("PlayerController");

    // ---------------------------------------------------------
    // 3. GAME CAMERA & LIGHT (Aynen Korundu)
    // ---------------------------------------------------------
    entity gameCameraEntity = editorScene.create_entity();
    editorScene.get_registry().add_component<name_component>(gameCameraEntity, name_component{ "MainCamera" });
    editorScene.get_registry().add_component<transform_component>(gameCameraEntity, transform_component{ glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f) });
    editorScene.get_registry().add_component<camera_component>(gameCameraEntity, camera_component{ 45.0f, 0.1f, 1000.0f, true });

    auto& camScriptComp = editorScene.get_registry().add_component<native_script_component>(gameCameraEntity);
    //camScriptComp.bind<camera_controller>("CameraController");

    entity lightEntity = editorScene.create_entity();
    editorScene.get_registry().add_component<name_component>(lightEntity, name_component{ "Main Light" });
    editorScene.get_registry().add_component<transform_component>(lightEntity, transform_component{ glm::vec3(20.0f, 50.0f, 20.0f), glm::vec3(0.0f), glm::vec3(1.0f) });

    light_component pointLight;
    pointLight.type = light_type::spot;
    pointLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
    pointLight.intensity = 4.0f;
    pointLight.range = 200.0f;
    pointLight.inner_cone = 10.0f;
    pointLight.outer_cone = 30.0f;
    editorScene.get_registry().add_component<light_component>(lightEntity, pointLight);
}