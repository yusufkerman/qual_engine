#include "editor_layer.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"

#include "editor_camera.h"

#include <iostream>

static void update_editor_camera(EngineContext& ctx, EditorContext& editorCtx)
{
    if (!editorCtx.camera)
        return;

    editorCtx.camera->update(
        ctx.window,
        ctx.inputState,
        ctx.sensitivity,
        ctx.moveSpeed,
        ctx.vestaSys.viewport_state().scene().focused
    );
}

static void update_viewports(EngineContext& ctx)
{
    auto& viewportState = ctx.vestaSys.viewport_state();

    const auto& scene = viewportState.scene();
    if (scene.size.x > 1.0f && scene.size.y > 1.0f)
    {
        ctx.editorTarget.resize(
            static_cast<int>(scene.size.x),
            static_cast<int>(scene.size.y)
        );
    }

    const auto& game = viewportState.game();
    if (game.size.x > 1.0f && game.size.y > 1.0f)
    {
        auto& gameState = viewportState.game();

        constexpr float renderScale = 1.0f;

        gameState.render_size.x =
            std::max(1, (int)(gameState.size.x * renderScale));

        gameState.render_size.y =
            std::max(1, (int)(gameState.size.y * renderScale));

        ctx.gameTarget.resize(
            gameState.render_size.x,
            gameState.render_size.y
        );
    }
}

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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.screenW, ctx.screenH);

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    editorCtx.sceneTarget = &ctx.editorTarget;
    editorCtx.gameTarget = &ctx.gameTarget;

    ctx.uiManager.beginFrame();

    ctx.vestaSys.update();

    ctx.uiManager.endFrame();
}