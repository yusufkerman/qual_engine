#pragma once

/**
 * @file editor_layer.h
 * @brief Motor döngüsü ile Vesta editör sistemi arasındaki yaşam döngüsü ve viewport güncellemelerini yöneten katman.
 */

struct EngineContext;
struct EditorContext;

/**
 * @class editor_layer
 * @brief Editör kamerasını, hedef doku (FBO) boyutlandırmalarını ve ImGui çerçeve akışını yöneten statik katman.
 */
class editor_layer
{
public:
    /**
     * @brief Editör kamerasını ve sahne/oyun pencere çözünürlüklerini günceller.
     * @param ctx Motorun çekirdek durum ve hedef doku referansları.
     * @param editorCtx Editör bağlamı ve kamera referansı.
     */
    static void update(EngineContext& ctx, EditorContext& editorCtx);

    /**
     * @brief Vesta editör panellerini çizer ve arka plan tamponlarını temizler.
     * @param ctx Motor çekirdek bağlamı ve kullanıcı arayüzü yöneticisi.
     * @param editorCtx Editör bağlamı.
     */
    static void render(EngineContext& ctx, EditorContext& editorCtx);
};