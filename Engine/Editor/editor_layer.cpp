/**
 * @file editor_layer.cpp
 * @brief Editör katmanının yaşam döngüsü, viewport boyut senkronizasyonu ve çizim uygulaması.
 */

#include "editor_layer.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"
#include "editor_camera.h"
#include <algorithm>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace {

    /**
     * @brief Sahne görünümü odaklandığında editör serbest uçuş kamerasını günceller.
     */
    void update_editor_camera(EngineContext& ctx, EditorContext& editorCtx)
    {
        if (!editorCtx.camera) {
            return;
        }

        const float frameScaledSpeed = ctx.moveSpeed * (ctx.time.unscaled_delta_time() * 60.0f);
        editorCtx.camera->update(
            ctx.window,
            ctx.inputState,
            ctx.sensitivity,
            frameScaledSpeed,
            ctx.vestaSys.viewport_state().scene().focused
        );
    }

    /**
     * @brief Viewport panellerinin anlık boyutlarına göre FBO render hedeflerini yeniden boyutlandırır.
     */
    void update_viewports(EngineContext& ctx)
    {
        auto& viewportState = ctx.vestaSys.viewport_state();
        const auto& ps1 = ctx.renderSettings;
        const bool use_ps1_downscale = ps1.enable_ps1_mode && ps1.ps1_downscale;

        // 1. Sahne (Scene / Editor Viewport) Hedef Çözünürlüğü:
        // Kullanıcı kuralı: Editör ekranı PS1 modu açık olsa dahi daima tam çözünürlükte kalır.
        const auto& scene = viewportState.scene();
        if (scene.size.x > 1.0f && scene.size.y > 1.0f)
        {
            ctx.editorTarget.resize(
                static_cast<int>(scene.size.x),
                static_cast<int>(scene.size.y)
            );
        }

        // 2. Oyun (Game Viewport) Hedef Çözünürlüğü:
        // Sadece oyun ekranında PS1 downscale uygulanır; Free Aspectte stretching engellenir.
        const auto& game = viewportState.game();
        if (game.size.x > 1.0f && game.size.y > 1.0f)
        {
            auto& gameState = viewportState.game();
            int gw = static_cast<int>(gameState.size.x);
            int gh = static_cast<int>(gameState.size.y);

            if (use_ps1_downscale)
            {
                const float aspect = (gameState.size.y > 0.0f) 
                    ? (gameState.size.x / gameState.size.y) 
                    : (16.0f / 9.0f);

                switch (ps1.ps1_resolution_preset)
                {
                case 0: // 240p PS1 Otantik (Free Aspect uyumlu - stretching yapmaz)
                    gh = 240;
                    gw = std::max(1, static_cast<int>(std::round(240.0f * aspect)));
                    break;
                case 1: // 320x240 Sabit (PS1 Klasik 4:3)
                    gw = 320;
                    gh = 240;
                    break;
                case 2: // 426x240 Sabit (PS1 Geniş Ekran 16:9)
                    gw = 426;
                    gh = 240;
                    break;
                case 3: // 480p Hi-Res PS1 (Free Aspect uyumlu)
                    gh = 480;
                    gw = std::max(1, static_cast<int>(std::round(480.0f * aspect)));
                    break;
                case 4: // Dinamik 0.25x Çeyrek Çözünürlük
                    gw = std::max(1, static_cast<int>(gameState.size.x * 0.25f));
                    gh = std::max(1, static_cast<int>(gameState.size.y * 0.25f));
                    break;
                case 5: // Özel Çözünürlük
                    gw = std::max(32, ps1.ps1_custom_width);
                    gh = std::max(32, ps1.ps1_custom_height);
                    break;
                default:
                    gh = 240;
                    gw = std::max(1, static_cast<int>(std::round(240.0f * aspect)));
                    break;
                }
            }

            gameState.render_size.x = gw;
            gameState.render_size.y = gh;
            ctx.gameTarget.resize(gw, gh);
        }
    }

} // anonymous namespace

void editor_layer::update(EngineContext& ctx, EditorContext& editorCtx)
{
    auto& viewportState = ctx.vestaSys.viewport_state();

    if (viewportState.scene().focused || (editorCtx.camera && editorCtx.camera->is_focus_active()))
    {
        update_editor_camera(ctx, editorCtx);
    }

    update_viewports(ctx);
}

void editor_layer::render(EngineContext& ctx, EditorContext& editorCtx)
{
    // Varsayılan ekran çerçevesini hazırla
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.screenW, ctx.screenH);

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    editorCtx.sceneTarget = &ctx.editorTarget;
    editorCtx.gameTarget = &ctx.gameTarget;

    // ImGui / Vesta arayüz çerçevesini başlat ve çiz
    ctx.uiManager.beginFrame([&ctx, &editorCtx]() {
        ctx.vestaSys.render_main_menu_bar(ctx, editorCtx);
    });
    ctx.vestaSys.update();
    ctx.uiManager.endFrame();
}