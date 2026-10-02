#pragma once

/**
 * @file runtime_layer.h
 * @brief Oyun dünyası simülasyonunu, kamera kontrollerini ve fizik güncelleme döngüsünü yöneten katman.
 */

struct EngineContext;
struct EditorContext;

/**
 * @class runtime_layer
 * @brief PLAY modunda sahne simülasyonunu ve sistem senkronizasyonunu icra eden statik servis katmanı.
 */
class runtime_layer
{
public:
    runtime_layer() = delete;

    /**
     * @brief Oyun sahnesini bir kare simüle eder. Yalnızca EngineMode::PLAY aktifken çalışır.
     *
     * @param ctx Motor bağlamı, oturum (session) ve fizik sistemi referansları.
     * @param editorCtx Editör çalışma durumu ve mod bilgisi (PLAY/EDIT).
     */
    static void update(EngineContext& ctx, EditorContext& editorCtx);
};