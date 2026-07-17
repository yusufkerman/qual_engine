#include "ui_pass.h"

#include "../../Engine/engine_context.h"
#include "../../Editor/editor_context.h"

void ui_pass::render(EngineContext& ctx, EditorContext& editorCtx)
{
    glViewport(0, 0, ctx.screenW, ctx.screenH);

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    editorCtx.setSceneTexture(ctx.editorTarget.tex);
    editorCtx.setGameTexture(ctx.gameTarget.tex);

    ctx.uiManager.beginFrame();

    scene& uiScene = ctx.session.is_playing()
        ? ctx.session.runtime_scene()
        : ctx.session.editor_scene();

    //ctx.editor.renderUI(uiScene,ctx, editorCtx);

    ctx.uiManager.endFrame();

    glEnable(GL_DEPTH_TEST);
}