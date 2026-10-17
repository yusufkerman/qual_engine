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

        // 1. Sahne (Scene) Hedef Çözünürlüğü
        const auto& scene = viewportState.scene();
        if (scene.size.x > 1.0f && scene.size.y > 1.0f)
        {
            ctx.editorTarget.resize(
                static_cast<int>(scene.size.x),
                static_cast<int>(scene.size.y)
            );
        }

        // 2. Oyun (Game) Hedef Çözünürlüğü
        const auto& game = viewportState.game();
        if (game.size.x > 1.0f && game.size.y > 1.0f)
        {
            auto& gameState = viewportState.game();
            constexpr float renderScale = 1.0f;

            gameState.render_size.x = std::max(1, static_cast<int>(gameState.size.x * renderScale));
            gameState.render_size.y = std::max(1, static_cast<int>(gameState.size.y * renderScale));

            ctx.gameTarget.resize(
                gameState.render_size.x,
                gameState.render_size.y
            );
        }
    }

} // anonymous namespace

void editor_layer::update(EngineContext& ctx, EditorContext& editorCtx)
{
    auto& viewportState = ctx.vestaSys.viewport_state();

    if (viewportState.scene().focused)
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