#pragma once

/**
 * @file ui_pass.h
 * @brief Editör arayüzü ve ImGui katmanının ekrana yansıtıldığı son render geçişini tanımlar.
 */

struct EngineContext;
struct EditorContext;

namespace ui_pass
{
    /**
     * @brief Ekran tamponunu temizler, viewport dokularını bağlar ve Vesta UI çerçevesini çizer.
     *
     * @param ctx Motorun ana sistem, pencere ve arayüz yöneticisi referansları.
     * @param editorCtx Editör çalışma zamanı bağlamı ve hedef doku tanıtıcıları.
     */
    void render(EngineContext& ctx, EditorContext& editorCtx);
}