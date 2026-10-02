#pragma once

/**
 * @file editor_pass.h
 * @brief Editör sahne görünümü (Scene Viewport) render geçişini tanımlar.
 */

struct EngineContext;
struct EditorContext;

namespace editor_pass
{
    /**
     * @brief Editör sahne FBO'sunu temizler, Skybox, sahne geometrisi ve hata ayıklama gizmo'larını çizer.
     *
     * @param ctx Motorun ana sistem, render ve fizik referansları.
     * @param editorCtx Editör çalışma zamanı bağlamı ve serbest uçuş kamerası.
     */
    void render(EngineContext& ctx, EditorContext& editorCtx);
}