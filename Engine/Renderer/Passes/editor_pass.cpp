#include "editor_pass.h"
#include "../../Engine/engine_context.h"
#include "skybox_pass.h"
#include "../../Systems/physics_api.h"
#include <glm/gtc/matrix_transform.hpp>

void editor_pass::render(EngineContext& ctx)
{
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.editorTarget.fbo);
    glViewport(0, 0, ctx.editorTarget.width, ctx.editorTarget.height);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 editorView = ctx.editor.editorCamera.getViewMatrix();
    const glm::mat4 editorProj = ctx.editor.editorCamera.getProjectionMatrix(
        (float)ctx.editorTarget.width / (float)ctx.editorTarget.height
    );

    skybox_pass::render(ctx, editorView, editorProj);

    ctx.renderSystem.render(
        ctx.session.editor_scene().get_registry(),
        ctx.session.editor_scene().get_light_batch(),
        ctx,
        editorView,
        editorProj
    );


    // ========================================================================
    // GIZMO SAHNE BAĞLANTISI
    // ========================================================================
    auto& reg = ctx.session.editor_scene().get_registry();
    int selectedID = ctx.editor.getSelectedEntity();

    if (selectedID != -1)
    {
        entity selectedEntity = (entity)selectedID;

        auto* transform = reg.get_component<transform_component>(selectedEntity);
        auto* collider = reg.get_component<collider_component>(selectedEntity);
        auto* rb = reg.get_component<rigidbody_component>(selectedEntity);

        if (transform && collider)
        {
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            bool physicsActive = false;

            // PLAY modunda Jolt'tan pozisyon + rotasyonu al
            if (rb && !rb->body_id.IsInvalid())
            {
                modelMatrix = ctx.physicsSystem.api().get_jolt_transform(selectedEntity);
                physicsActive = true;
            }

            // EDITOR modunda Transform componentini kullan
            if (!physicsActive)
            {
                modelMatrix = glm::translate(modelMatrix, transform->position);

                modelMatrix = glm::rotate(modelMatrix, transform->rotation.x, glm::vec3(1, 0, 0));
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.y, glm::vec3(0, 1, 0));
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.z, glm::vec3(0, 0, 1));
            }

            glm::vec4 gizmoColor(0.0f, 1.0f, 0.0f, 1.0f);

            switch (collider->type)
            {
            case collider_type::box:
                ctx.gizmoSystem.DrawWireCube(
                    modelMatrix,
                    collider->extents,
                    gizmoColor
                );
                break;

            case collider_type::sphere:
                ctx.gizmoSystem.DrawWireSphere(
                    modelMatrix,
                    collider->extents.x,   // Sphere yarıçapı
                    gizmoColor
                );
                break;

            default:
                break;
            }
        }
    }

    // ========================================================================
    // 🚀 ARIA GIZMO FLUSH KATMANI
    // ========================================================================
    // Eski manager yerine doğrudan Aria Cache'e gidip GPU üzerindeki hazır shader ID'sini çekiyoruz
    unsigned int gizmoGLShaderID = ctx.aria_system->get_shader_id(ctx.gizmoShaderHandle);

    // Çizim komutlarını GPU'ya fırlatıyoruz
    ctx.gizmoSystem.Flush(editorView, editorProj, gizmoGLShaderID);
}