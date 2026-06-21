#include "game_pass.h"

#include "../../Engine/engine_context.h"
#include "../../Engine/fly_camera.h"
#include "../../shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include "skybox_pass.h"

void game_pass::render(EngineContext& ctx)
{
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.gameTarget.fbo);
    glViewport(0, 0, ctx.gameTarget.width, ctx.gameTarget.height);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene& gameScene = ctx.session.game_scene();
    const entity gameCam = gameScene.get_active_camera();

    if (gameCam != 0 && gameScene.get_registry().has_component<camera_component>(gameCam))
    {
        auto* t = gameScene.get_registry().get_component<transform_component>(gameCam);
        auto* c = gameScene.get_registry().get_component<camera_component>(gameCam);

        if (t && c)
        {
            glm::vec3 forward = calc_forward(t->rotation);

            glm::mat4 gameView = glm::lookAt(
                t->position,
                t->position + forward,
                glm::vec3(0, 1, 0)
            );

            glm::mat4 gameProj = glm::perspective(
                glm::radians(c->fov),
                (float)ctx.gameTarget.width / (float)ctx.gameTarget.height,
                c->nearPlane,
                c->farPlane
            );

            skybox_pass::render(ctx, gameView, gameProj);

            ctx.renderSystem.render(
                gameScene.get_registry(),
                ctx.session.game_scene().get_light_batch(),
                ctx.renderer,
                ctx.shaderManager,
                ctx.materialManager,
                gameView,
                gameProj
            );
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}