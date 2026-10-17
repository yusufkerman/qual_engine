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
     * @brief Sabit zaman adımlı fizik simülasyonunu ve ECS senkronizasyonunu icra eder.
     * Glenn Fiedler Accumulator döngüsünden sabit zaman adımıyla çağrılır.
     *
     * @param ctx Motor bağlamı.
     * @param editorCtx Editör bağlamı.
     * @param fixedDeltaTime Sabit simülasyon adımı (sn).
     */
    static void fixed_update(EngineContext& ctx, EditorContext& editorCtx, float fixedDeltaTime);

    /**
     * @brief Geriye dönük uyumluluk güncelleme fonksiyonu.
     */
    static void update(EngineContext& ctx, EditorContext& editorCtx);
};