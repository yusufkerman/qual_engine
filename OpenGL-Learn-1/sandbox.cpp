#include "sandbox.h"

#include "Engine/engine_context.h"
#include "Importers/ModelLoader.h"

#include <glm/gtc/matrix_transform.hpp>

void sandbox_load_default_scene(EngineContext& ctx)
{
    scene& editorScene = ctx.session.editor_scene();

    ctx.editor.setScene(&editorScene);

    Model model = ModelLoader::Load("Assets/model1.FBX");
    unsigned int meshID = ctx.renderer.UploadModel(model);

    entity chairEntity = editorScene.create_entity();
    editorScene.get_registry().add_transform(chairEntity, {
        glm::vec3(0.0f),
        glm::vec3(glm::radians(-90.0f), glm::radians(0.0f), glm::radians(0.0f)),
        glm::vec3(1.0f)
        });
    editorScene.get_registry().add_mesh(chairEntity, { meshID });
	editorScene.get_registry().add_name(chairEntity, { "Chair" });

    entity gameCameraEntity = editorScene.create_entity();
    editorScene.get_registry().add_transform(gameCameraEntity, {
        glm::vec3(0.0f, -5.0f, 70.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f)
        });
    editorScene.get_registry().add_camera(gameCameraEntity, {
        45.0f,
        0.1f,
        1000.0f
        });
    editorScene.set_active_camera(gameCameraEntity);
    editorScene.get_registry().add_name(gameCameraEntity, { "Game Camera" });


    entity lightEntity = editorScene.create_entity();
    editorScene.get_registry().add_transform(lightEntity, {
        glm::vec3(25.0f, 110.0f, 5.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f)
        });
    editorScene.get_registry().add_light(lightEntity, {});
    editorScene.get_registry().add_name(lightEntity, { "Point Light" });

}