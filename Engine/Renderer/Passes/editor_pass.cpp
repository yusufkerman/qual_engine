/**
 * @file editor_pass.cpp
 * @brief Editör sahne çizim geçişi, FBO yönetimi ve fiziksel sınır kutusu (Collider) gizmo çizim uygulaması.
 */

#include "editor_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Editor/editor_context.h"
#include "../../Editor/editor_camera.h"
#include "skybox_pass.h"
#include "../../Systems/physics_api.h"
#include "../../Editor/Vesta/vesta.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

void editor_pass::render(EngineContext& ctx, EditorContext& editorCtx)
{
    if (!editorCtx.camera) {
        return;
    }

    // 1. Hedef Framebuffer ve Viewport yapılandırması
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.editorTarget.fbo);
    glViewport(0, 0, ctx.editorTarget.width, ctx.editorTarget.height);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Kamera matrislerini hesapla (sıfıra bölme koruması ile)
    const float aspect = static_cast<float>(ctx.editorTarget.width) /
        static_cast<float>(std::max(1, ctx.editorTarget.height));

    const glm::mat4 editorView = editorCtx.camera->getViewMatrix();
    const glm::mat4 editorProj = editorCtx.camera->getProjectionMatrix(aspect);

    // 3. Gökyüzü (Skybox) render geçişi
    skybox_pass::render(ctx, editorView, editorProj);

    // 4. Ana sahne geometrisi ve ışıklandırma
    ctx.renderSystem.render(
        ctx.session.editor_scene().get_registry(),
        ctx.session.editor_scene().get_light_batch(),
        ctx,
        editorView,
        editorProj
    );

    // 5. Seçili varlık için sınır kutusu / çarpıştırıcı (Collider Gizmo) çizimi
    auto& reg = ctx.session.editor_scene().get_registry();
    const int selectedID = ctx.vestaSys.selection().get_first_selected_entity();

    if (selectedID != -1)
    {
        const auto selectedEntity = static_cast<entity>(selectedID);

        const auto* transform = reg.get_component<transform_component>(selectedEntity);
        const auto* collider = reg.get_component<collider_component>(selectedEntity);
        const auto* rb = reg.get_component<rigidbody_component>(selectedEntity);

        if (transform && collider)
        {
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            bool physicsActive = false;

            // Simülasyon modunda dönüşümü fizik motorundan (Jolt) al
            if (rb && !rb->body_id.IsInvalid())
            {
                modelMatrix = ctx.physicsSystem.api().get_jolt_transform(selectedEntity);
                physicsActive = true;
            }

            // Editör modunda yerel dönüşüm bileşenini kullan
            if (!physicsActive)
            {
                modelMatrix = glm::translate(modelMatrix, transform->position);
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            }

            constexpr glm::vec4 gizmoColor(0.0f, 1.0f, 0.0f, 1.0f);

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
                    collider->extents.x,
                    gizmoColor
                );
                break;

            default:
                break;
            }
        }
    }

    // 6. Aria varlık önbelleğinden gizmo shader programını al ve çizgileri ekrana boşalt
    if (ctx.aria_system)
    {
        const unsigned int gizmoGLShaderID = ctx.aria_system->get_shader_id(ctx.gizmoShaderHandle);
        ctx.gizmoSystem.Flush(editorView, editorProj, gizmoGLShaderID);
    }
}