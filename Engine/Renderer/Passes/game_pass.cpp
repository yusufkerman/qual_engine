#include "game_pass.h"

#include "../../Engine/engine_context.h"
#include "../../Engine/fly_camera.h"
#include "../../shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include "skybox_pass.h"

void game_pass::render(EngineContext& ctx)
{
    // 1. Viewport boyutlarını dinamik olarak takip etmek için değişkenler
    int currentWidth = 0;
    int currentHeight = 0;

    if (ctx.isEditorBuild)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ctx.gameTarget.fbo);
        glViewport(0, 0, ctx.gameTarget.width, ctx.gameTarget.height);

        currentWidth = ctx.gameTarget.width;
        currentHeight = ctx.gameTarget.height;
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, ctx.screenW, ctx.screenH);

        currentWidth = ctx.screenW;
        currentHeight = ctx.screenH;
    }

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

            // 2. 🔥 STRETCH ÖNLEYİCİ: En-boy oranını (Aspect Ratio) o anki aktif boyutlara göre hesapla
            // Sıfıra bölünme hatasını (Crash) önlemek için koruma ekledik.
            float aspectRatio = 1.6f; // Fallback varsayılan 16:10
            if (currentHeight > 0)
            {
                aspectRatio = (float)currentWidth / (float)currentHeight;
            }

            glm::mat4 gameProj = glm::perspective(
                glm::radians(c->fov),
                aspectRatio, // Artık sabit değil, dinamik en-boy oranı!
                c->nearPlane,
                c->farPlane
            );

            skybox_pass::render(ctx, gameView, gameProj);

            ctx.renderSystem.render(
                gameScene.get_registry(),
                gameScene.get_light_batch(),
                ctx,
                gameView,
                gameProj
            );
        }
    }

    if (ctx.isEditorBuild)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}