#include "ui_pass.h"

#include "../../Engine/engine_context.h"

void ui_pass::render(EngineContext& ctx)
{
    glViewport(0, 0, ctx.screenW, ctx.screenH);

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    ctx.editor.setSceneTexture(ctx.editorTarget.tex);
    ctx.editor.setGameTexture(ctx.gameTarget.tex);

    ctx.uiManager.beginFrame();

    scene& uiScene = ctx.session.is_playing()
        ? ctx.session.runtime_scene()
        : ctx.session.editor_scene();

    ctx.editor.renderUI(uiScene);

    ctx.uiManager.endFrame();
}