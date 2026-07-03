#include "grid_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/fly_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

void grid_pass::render(EngineContext& ctx)
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ========================================================================
    // 🚀 ARIA SHADER KATMANI
    // ========================================================================
    // Eski shaderManager yerine Aria Cache'den GPU üzerindeki hazır shader ID'sini çekiyoruz
    unsigned int gridGLShaderID = ctx.aria_system->get_shader_id(ctx.gridShaderHandle);
    glUseProgram(gridGLShaderID);

    const glm::vec3& camPos = ctx.editor.editorCamera.position;
    const glm::vec3 gridForward = calc_forward(ctx.editor.editorCamera.rotation);

    const glm::mat4 gridView = glm::lookAt(
        camPos,
        camPos + gridForward,
        glm::vec3(0, 1, 0)
    );

    const glm::mat4 gridProj = glm::perspective(
        glm::radians(ctx.editor.editorCamera.fov),
        (float)ctx.editorTarget.width / (float)ctx.editorTarget.height,
        ctx.editor.editorCamera.nearPlane,
        ctx.editor.editorCamera.farPlane
    );

    const glm::mat4 invViewProj = glm::inverse(gridProj * gridView);

    // 🚀 Doğrudan OpenGL uniform çağrıları ile performans kazanıyoruz
    glUniformMatrix4fv(glGetUniformLocation(gridGLShaderID, "invViewProj"), 1, GL_FALSE, glm::value_ptr(invViewProj));
    glUniform3fv(glGetUniformLocation(gridGLShaderID, "cameraPos"), 1, glm::value_ptr(camPos));

    ctx.grid.draw();

    glUseProgram(0); // Temizlik
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glEnable(GL_DEPTH_TEST);
}