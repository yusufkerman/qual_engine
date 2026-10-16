#pragma once

/**
 * @file gizmo_pass.h
 * @brief Editör sahnesindeki bileşen gizmolarını (Collider, Kamera Frustum, Işık Menzil/Koni) ve çalışma zamanı hata ayıklama çizgilerini çizen modüler render geçişi.
 */

struct EngineContext;
struct EditorContext;

class gizmo_pass
{
public:
    /**
     * @brief Aktif gizmo ayarlarına göre tüm 3B sahne gizmolarını toplar ve GPU'ya çizdirir.
     *
     * @param ctx Motor bağlamı.
     * @param editorCtx Editör bağlamı.
     */
    static void render(EngineContext& ctx, EditorContext& editorCtx);
};

