#include "editor_pass.h"

#include "../../Engine/engine_context.h"
#include "../../shader.h"
#include "skybox_pass.h"

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
    // GIZMO RENDERING (Immediate Mode Test)
    // ========================================================================

    // Test Çizim İsteklerini Topla
    ctx.gizmoSystem.DrawLine(glm::vec3(0.0f), glm::vec3(10.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    ctx.gizmoSystem.DrawWireCube(glm::vec3(0.0f), glm::vec3(10.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Ekrana bas
    shader& gizmoShader = ctx.shaderManager.get(ctx.gizmoShaderID);
    ctx.gizmoSystem.Flush(editorView, editorProj, gizmoShader.ID);

}