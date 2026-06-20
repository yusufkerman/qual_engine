#include "sandbox.h"

#include "Engine/engine_context.h"
#include "Importers/ModelLoader.h"

#include <glm/gtc/matrix_transform.hpp>

void sandbox_load_default_scene(EngineContext& ctx)
{
    scene& editorScene = ctx.session.editor_scene();
    ctx.editor.setScene(&editorScene);

    // =========================
    // MODEL LOAD
    // =========================
    Model model = ModelLoader::Load("Assets/model1.FBX");
    unsigned int meshID = ctx.renderer.UploadModel(model);

    // MATERIAL (örnek)
    material mat;
    mat.shader_id = 0;
    mat.base_color = glm::vec3(1.0f, 1.0f, 1.0f);
    mat.diffuse_texture = 0;

    unsigned int materialID = ctx.materialManager.create(mat);

    // =========================
    // CHAIR ENTITY
    // =========================
    entity chairEntity = editorScene.create_entity();

    editorScene.get_registry().add_component<transform_component>(
        chairEntity,
        transform_component{
            glm::vec3(0.0f),
            glm::vec3(
                glm::radians(-90.0f),
                glm::radians(0.0f),
                glm::radians(0.0f)
            ),
            glm::vec3(1.0f)
        }
    );

    editorScene.get_registry().add_component<mesh_component>(
        chairEntity,
        mesh_component{ meshID }
    );

    editorScene.get_registry().add_component<material_component>(
        chairEntity,
        material_component{ materialID }
    );

    editorScene.get_registry().add_component<bounds_component>(
        chairEntity,
        bounds_component{
            model.meshes[0].localCenter,
            model.meshes[0].localRadius
        }
    );

    editorScene.get_registry().add_component<name_component>(
        chairEntity,
        name_component{ "Chair" }
    );

    // =========================
    // CAMERA
    // =========================
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

    // =========================
    // LIGHT
    // =========================
    entity lightEntity = editorScene.create_entity();

    editorScene.get_registry().add_component<transform_component>(
        lightEntity,
        transform_component{
            glm::vec3(25.0f, 110.0f, 5.0f),
            glm::vec3(0.0f),
            glm::vec3(1.0f)
        }
    );

    editorScene.get_registry().add_component<light_component>(
        lightEntity,
        light_component{}
    );

    editorScene.get_registry().add_component<name_component>(
        lightEntity,
        name_component{ "Point Light" }
    );
}