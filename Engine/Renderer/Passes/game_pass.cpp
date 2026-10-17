/**
 * @file game_pass.cpp
 * @brief Oyun kamerası projeksiyon hesaplamaları, skybox ve sahne çizim uygulaması.
 */

#include "game_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/fly_camera.h"
#include "skybox_pass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

void game_pass::render(EngineContext& ctx)
{
    int currentWidth = 0;
    int currentHeight = 0;

    // 1. Hedef Framebuffer ve Viewport sınırlarını belirle
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
    glClearColor(
        ctx.renderSettings.clear_color_game.r,
        ctx.renderSettings.clear_color_game.g,
        ctx.renderSettings.clear_color_game.b,
        1.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Sahnedeki aktif oyun kamerasını sorgula
    scene& gameScene = ctx.session.game_scene();
    const entity gameCam = gameScene.get_active_camera();

    if (gameCam != 0 && gameScene.get_registry().has_component<camera_component>(gameCam))
    {
        const auto* t = gameScene.get_registry().get_component<transform_component>(gameCam);
        const auto* c = gameScene.get_registry().get_component<camera_component>(gameCam);

        if (t && c)
        {
            const glm::vec3 forward = calc_forward(t->rotation);

            const glm::mat4 gameView = glm::lookAt(
                t->position,
                t->position + forward,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );

            // Dinamik en/boy oranı (sıfıra bölme koruması ile)
            float aspectRatio = 16.0f / 9.0f;
            if (currentHeight > 0)
            {
                aspectRatio = static_cast<float>(currentWidth) / static_cast<float>(currentHeight);
            }

            const glm::mat4 gameProj = glm::perspective(
                glm::radians(c->fov),
                aspectRatio,
                c->nearPlane,
                c->farPlane
            );

            // Gökyüzü çizimi
            skybox_pass::render(ctx, gameView, gameProj);

            // Ana sahne çizimi (isSceneViewport = false -> Game Viewport)
            ctx.renderSystem.render(
                gameScene.get_registry(),
                gameScene.get_light_batch(),
                ctx,
                gameView,
                gameProj,
                false
            );

            // İsteğe bağlı çalışma zamanı (Gameplay API) gizmolarını oyun ekranında çiz
            if (ctx.aria_system && ctx.gizmoSystem.settings().enabled && ctx.gizmoSystem.settings().show_gameplay_gizmos_in_game)
            {
                const unsigned int gizmoShaderID = ctx.aria_system->get_shader_id(ctx.gizmoShaderHandle);
                ctx.gizmoSystem.RenderGameplayGizmos(gameView, gameProj, gizmoShaderID);
            }
        }
    }

    if (ctx.isEditorBuild)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}