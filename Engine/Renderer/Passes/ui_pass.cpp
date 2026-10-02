/**
 * @file ui_pass.cpp
 * @brief UI render geçişi, framebuffer sıfırlama ve ImGui çerçeve yaşam döngüsü uygulaması.
 */

#include "ui_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Editor/editor_context.h"

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

void ui_pass::render(EngineContext& ctx, EditorContext& editorCtx)
{
    // 1. Ekran çerçeve tamponunu bağla ve tüm ekranı kapla
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.screenW, ctx.screenH);

    // 2. UI çizimi için derinlik testini devre dışı bırak ve tamponu temizle
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    // 3. Editör panellerinde gösterilecek FBO doku kimliklerini güncelle
    editorCtx.setSceneTexture(ctx.editorTarget.tex);
    editorCtx.setGameTexture(ctx.gameTarget.tex);

    // 4. ImGui ve Vesta arayüz çerçevesini çiz
    ctx.uiManager.beginFrame();

    ctx.vestaSys.update();

    ctx.uiManager.endFrame();

    // 5. Grafik boru hattı derinlik testini tekrar aktif et
    glEnable(GL_DEPTH_TEST);
}