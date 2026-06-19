#include "grid_pass.h"

#include "../../Engine/engine_context.h"
#include "../../Engine/fly_camera.h"
#include "../../shader.h"

#include <glm/gtc/matrix_transform.hpp>

void grid_pass::render(EngineContext& ctx)
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ctx.gridShader->use();

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

    ctx.gridShader->setMat4("invViewProj", invViewProj);
    ctx.gridShader->setVec3("cameraPos", camPos);

    ctx.grid.draw();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}