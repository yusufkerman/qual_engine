#pragma once

/**
 * @file engine.h
 * @brief Oyun motorunun ana yaşam döngüsü (init, update, render, shutdown) fonksiyonlarını tanımlar.
 */

#include "engine_context.h"

namespace Engine
{
    /**
     * @brief Motoru ve gerekli tüm alt sistemleri başlatır.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param editorCtx Editör bağlam (context) nesnesi.
     * @return bool Başlatma başarılı olursa true, hata oluşursa false döner.
     */
    bool init(EngineContext& ctx, EditorContext& editorCtx);

    /**
     * @brief Motorun mantıksal güncellemelerini (girdi işlemleri, fizik, sahne yönetimi vb.) gerçekleştirir.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param editorCtx Editör bağlam (context) nesnesi.
     */
    void update(EngineContext& ctx, EditorContext& editorCtx);

    /**
     * @brief Ekrana çizilecek olan grafikleri (oyun görüntüleri ve editör arayüzü) işler.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param editorCtx Editör bağlam (context) nesnesi.
     */
    void render(EngineContext& ctx, EditorContext& editorCtx);

    /**
     * @brief Motoru güvenli bir şekilde kapatır ve tahsis edilen bellek ile GPU kaynaklarını temizler.
     *
     * @param ctx Motor bağlam (context) nesnesi.
     * @param editorCtx Editör bağlam (context) nesnesi.
     */
    void shutdown(EngineContext& ctx, EditorContext& editorCtx);
}