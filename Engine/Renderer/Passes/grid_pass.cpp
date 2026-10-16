/**
 * @file grid_pass.cpp
 * @brief Sonsuz ızgara render geçişi, derinlik testi, alfa karışımı ve uniform matris aktarım uygulaması.
 */

#include "grid_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Editor/editor_context.h"
#include "../../Editor/editor_camera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

void grid_pass::render(EngineContext& ctx, EditorContext& editorCtx)
{
    if (!editorCtx.camera || !ctx.aria_system) {
        return;
    }

    const auto& gizmoSettings = ctx.gizmoSystem.settings();
    if (!gizmoSettings.enabled || !gizmoSettings.show_grid) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ctx.editorTarget.fbo);
    glViewport(0, 0, ctx.editorTarget.width, ctx.editorTarget.height);

    // Derinlik testi açık tutulur ki sahnedeki 3B nesneler ızgarayı doğru şekilde kessin (Z-occlusion)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Aria önbelleğinden ızgara shader tanıtıcısını al ve bağla
    const unsigned int gridGLShaderID = ctx.aria_system->get_shader_id(ctx.gridShaderHandle);
    glUseProgram(gridGLShaderID);

    // 2. Kamera ve ters projeksiyon matrislerini hazırla
    const glm::vec3& camPos = editorCtx.camera->position;
    const glm::mat4 gridView = editorCtx.camera->getViewMatrix();

    const float aspect = static_cast<float>(ctx.editorTarget.width) /
        static_cast<float>(std::max(1, ctx.editorTarget.height));

    const glm::mat4 gridProj = editorCtx.camera->getProjectionMatrix(aspect);
    const glm::mat4 viewProj = gridProj * gridView;
    const glm::mat4 invViewProj = glm::inverse(viewProj);

    // 3. Shader uniform değişkenlerini güncelle
    const GLint viewProjLoc = glGetUniformLocation(gridGLShaderID, "viewProj");
    if (viewProjLoc != -1) {
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
    }

    const GLint invViewProjLoc = glGetUniformLocation(gridGLShaderID, "invViewProj");
    if (invViewProjLoc != -1) {
        glUniformMatrix4fv(invViewProjLoc, 1, GL_FALSE, glm::value_ptr(invViewProj));
    }

    const GLint camPosLoc = glGetUniformLocation(gridGLShaderID, "cameraPos");
    if (camPosLoc != -1) {
        glUniform3fv(camPosLoc, 1, glm::value_ptr(camPos));
    }

    const GLint opacityLoc = glGetUniformLocation(gridGLShaderID, "u_GridOpacity");
    if (opacityLoc != -1) {
        glUniform1f(opacityLoc, gizmoSettings.grid_opacity);
    }

    const GLint showAxesLoc = glGetUniformLocation(gridGLShaderID, "u_ShowAxes");
    if (showAxesLoc != -1) {
        glUniform1i(showAxesLoc, gizmoSettings.show_grid_axes ? 1 : 0);
    }

    // 4. Quad geometrisini çiz
    ctx.grid.draw();

    // 5. OpenGL durumlarını varsayılana geri yükle
    glUseProgram(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
}