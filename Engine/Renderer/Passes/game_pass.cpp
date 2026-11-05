/**
 * @file game_pass.cpp
 * @brief Oyun kamerası projeksiyon hesaplamaları, skybox ve sahne çizim uygulaması.
 */

#include "game_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/fly_camera.h"
#include "../../ECS/transform_utils.h"
#include "skybox_pass.h"
#include "ssao_pass.h"
#include "volumetric_pass.h"
#include "post_process_pass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

void game_pass::render(EngineContext& ctx)
{
    int currentWidth = 0;
    int currentHeight = 0;

    // 1. Hedef Framebuffer ve Viewport sınırlarını belirle (Hem editörde hem standalone'da gameTarget'a çiz)
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.gameTarget.fbo);
    glViewport(0, 0, ctx.gameTarget.width, ctx.gameTarget.height);

    currentWidth = ctx.gameTarget.width;
    currentHeight = ctx.gameTarget.height;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(
        ctx.renderSettings.clear_color_game.r,
        ctx.renderSettings.clear_color_game.g,
        ctx.renderSettings.clear_color_game.b,
        0.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Sahnedeki aktif oyun kamerasını sorgula
    scene& gameScene = ctx.session.game_scene();
    registry& reg = gameScene.get_registry();
    entity gameCam = gameScene.get_active_camera();

    // Fallback 1: Eğer aktif kamera tanımlı değilse veya kamera bileşeni yoksa, sahnedeki ilk geçerli kamerayı bul
    if (gameCam == 0 || !reg.has_component<camera_component>(gameCam))
    {
        for (const entity e : reg.get_all_entities())
        {
            if (reg.has_component<camera_component>(e))
            {
                gameCam = e;
                gameScene.set_active_camera(e);
                break;
            }
        }
    }

    if (gameCam != 0 && reg.has_component<camera_component>(gameCam))
    {
        const auto* t = reg.get_component<transform_component>(gameCam);
        const auto* c = reg.get_component<camera_component>(gameCam);

        if (t && c)
        {
            // Kamera hiyerarsisindeki kesin dunya matrisini hesapla
            const glm::mat4 camWorld = ecs_transform::get_world_matrix(gameCam, reg);
            const glm::vec3 camPos = glm::vec3(camWorld[3]);
            glm::vec3 forward = -glm::vec3(camWorld[2]);
            if (glm::length(forward) > 0.0001f) forward = glm::normalize(forward);
            else forward = glm::vec3(0.0f, 0.0f, -1.0f);

            glm::vec3 up = glm::vec3(camWorld[1]);
            if (glm::length(up) > 0.0001f) up = glm::normalize(up);
            else up = glm::vec3(0.0f, 1.0f, 0.0f);

            const glm::mat4 gameView = glm::lookAt(
                camPos,
                camPos + forward,
                up
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

            const glm::mat4 jitteredProj = post_process_pass::apply_taa_jitter(
                ctx, gameProj, currentWidth, currentHeight, false
            );

            // Gökyüzü çizimi
            skybox_pass::render(ctx, gameView, jitteredProj);

            // Ana sahne çizimi (isSceneViewport = false -> Game Viewport)
            ctx.renderSystem.render(
                reg,
                gameScene.get_light_batch(),
                ctx,
                gameView,
                jitteredProj,
                false
            );

            // Ekran Uzayı Ortam Örtmesi (SSAO/GTAO) ve Küresel Aydınlatma (SSGI)
            ssao_pass::render(ctx, ctx.gameTarget, gameView, gameProj, false);

            // Hacimsel Işıklandırma (Volumetric Lighting / God Rays)
            volumetric_pass::render(ctx, ctx.gameTarget, gameView, gameProj, camPos, false);

            // HDR Ton Eşleme (Tone Mapping), sRGB Dönüşümü ve Kenar Yumuşatma (FXAA / TAA)
            post_process_pass::render(ctx, ctx.gameTarget, gameView, gameProj, false);

            // İsteğe bağlı çalışma zamanı (Gameplay API) gizmolarını oyun ekranında çiz
            if (ctx.aria_system && ctx.gizmoSystem.settings().enabled && ctx.gizmoSystem.settings().show_gameplay_gizmos_in_game)
            {
                const unsigned int gizmoShaderID = ctx.aria_system->get_shader_id(ctx.gizmoShaderHandle);
                ctx.gizmoSystem.RenderGameplayGizmos(gameView, gameProj, gizmoShaderID);
            }
        }
    }
    else
    {
        // Sahil fallback: Eğer sahnede hiç kamera yoksa bile ekran kapkaranlık kalmasın
        float aspectRatio = (currentHeight > 0) ? (static_cast<float>(currentWidth) / static_cast<float>(currentHeight)) : (16.0f / 9.0f);
        const glm::mat4 fallbackView = glm::lookAt(
            glm::vec3(0.0f, 5.0f, 10.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        const glm::mat4 fallbackProj = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 500.0f);
        const glm::mat4 jitteredFallbackProj = post_process_pass::apply_taa_jitter(
            ctx, fallbackProj, currentWidth, currentHeight, false
        );
        skybox_pass::render(ctx, fallbackView, jitteredFallbackProj);
        ctx.renderSystem.render(reg, gameScene.get_light_batch(), ctx, fallbackView, jitteredFallbackProj, false);
        ssao_pass::render(ctx, ctx.gameTarget, fallbackView, fallbackProj, false);
        volumetric_pass::render(ctx, ctx.gameTarget, fallbackView, fallbackProj, glm::vec3(0.0f, 5.0f, 10.0f), false);
        post_process_pass::render(ctx, ctx.gameTarget, fallbackView, fallbackProj, false);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
