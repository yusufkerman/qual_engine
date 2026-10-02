#pragma once

/**
 * @file grid_pass.h
 * @brief Editör sahne görünümü sonsuz zemin ızgarası (Infinite Grid) render geçişini tanımlar.
 */

struct EngineContext;
struct EditorContext;

namespace grid_pass
{
    /**
     * @brief Sonsuz zemin ızgarasını ters görünüm-projeksiyon matrisi kullanarak ekrana çizer.
     *
     * @param ctx Motor bağlamı, hedef çözünürlük ve ızgara çizici referansı.
     * @param editorCtx Editör bağlamı ve serbest uçuş kamerası.
     */
    void render(EngineContext& ctx, EditorContext& editorCtx);
}