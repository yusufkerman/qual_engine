/**
 * @file inspector_panel.h
 * @brief Seçili varlığa (entity) ait bileşenleri dinamik olarak listeleyen ana editör paneli.
 *
 * Bu panel; `component_ui_registry` içinde kayıtlı olan tüm bileşen arayüz sistemlerini
 * tarayarak, seçili varlığın sahip olduğu bileşenleri çalışma zamanında (runtime) tespit eder
 * ve ilgili ImGui çizim fonksiyonlarını ardışık olarak çağırır.
 */

#pragma once
#include "../../Scene/scene.h"

 // Forward declarations (Ön Bildirimler) - Sıfır Derleme Bağımlılığı kuralı
struct EngineContext;
struct EditorContext;

/**
 * @brief Editörün sağ veya sol tarafında konumlanan Nesne Özellikleri (Inspector) panel sınıfı.
 */
class inspector_panel
{
public:
    /**
     * @brief Aktif sahne üzerindeki seçili varlığın tüm bileşen arayüzlerini çizer.
     * * Fonksiyon, arka planda veri odaklı haritayı (`get_registry`) dönerek, şablon (template)
     * krizlerine girmeden tamamen tip-gizlenmiş (type-erased) fonksiyon göstergeleri üzerinden çalışır.
     *
     * @param scene Üzerinde arama yapılacak ve registry referansı barındıran aktif sahne nesnesi.
     * @param selectedEntity Hiyerarşi panelinden seçilmiş olan mevcut varlığın benzersiz kimliği (ID).
     * @param engineCtx Motorun genel çalışma zamanı bağlamı ve alt sistemleri (EngineContext).
     * @param editorCtx Editörün arayüz durumu ve panel odak bilgileri (EditorContext).
     */
    void render(scene& scene,
        int selectedEntity,
        EngineContext& engineCtx,
        EditorContext& editorCtx);
};