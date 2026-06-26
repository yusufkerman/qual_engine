#include "editor_layer.h"
#include "../Engine/engine_context.h"
#include <iostream>

static void update_editor_camera(EngineContext& ctx)
{
    ctx.editor.editorCamera.update(
        ctx.window,
        ctx.inputState,
        ctx.sensitivity,
        ctx.moveSpeed
    );
}

static void update_viewports(EngineContext& ctx)
{
    if (ctx.editor.sceneViewportSize.x > 1 && ctx.editor.sceneViewportSize.y > 1)
    {
        ctx.editorTarget.resize(
            (int)ctx.editor.sceneViewportSize.x,
            (int)ctx.editor.sceneViewportSize.y
        );
    }

    if (ctx.editor.gameViewportSize.x > 1 && ctx.editor.gameViewportSize.y > 1)
    {
        ctx.gameTarget.resize(
            (int)ctx.editor.gameViewportSize.x,
            (int)ctx.editor.gameViewportSize.y
        );
    }
}

void editor_layer::update(EngineContext& ctx, EditorContext& editorCtx)
{
    if (editorCtx.focusedPanel == FocusedPanel::SceneViewport) 
    {
        update_editor_camera(ctx);
    }

    update_viewports(ctx);

    ctx.editor.update(ctx.session.editor_scene());
}
void editor_layer::render(EngineContext& ctx, EditorContext& editorCtx)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.screenW, ctx.screenH);

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    ctx.editor.setSceneTexture(ctx.editorTarget.tex);
    ctx.editor.setGameTexture(ctx.gameTarget.tex);

    ctx.uiManager.beginFrame();

    scene& uiScene = ctx.session.is_playing()
        ? ctx.session.runtime_scene()
        : ctx.session.editor_scene();

    ctx.editor.renderUI(uiScene, ctx, editorCtx);

    ctx.uiManager.endFrame();
}