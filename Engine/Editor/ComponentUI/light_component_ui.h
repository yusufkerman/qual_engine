/**
 * @file light_component_ui.h
 * @brief Işık bileşeni (light_component) için editör kullanıcı arayüzü yönetim sistemi.
 * * Bu dosya; sahnedeki yönlü (Directional), noktasal (Point) ve spot (Spot) ışık kaynaklarının
 * renk, yoğunluk, menzil ve koni açıları gibi grafiksel parametrelerinin ImGui panellerinde
 * gerçek zamanlı manipüle edilmesini ve merkezi arayüz sistemine kaydedilmesini sağlar.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) ışık bileşeni verilerini ImGui üzerinde çizip günceller.
  * * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı (EngineContext).
  * @param editorCtx Editörün panel odakları ve arayüz durumu (EditorContext).
  */
inline void render_light_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    // Varlığa ait ışık bileşeninin ham göstergesini (pointer) güvenli bir şekilde çekiyoruz
    auto* l = reg.get_component<light_component>(e);
    if (l) {
        // 1. RENK VE YOĞUNLUK AYARLARI
        // Işığın RGB renk vektörünü değiştirmek için renk paleti seçici açar
        ImGui::ColorEdit3("Color", &l->color.x);

        // Işık şiddeti (Intensity) - 0.1'lik adımlarla hassas kaydırma
        ImGui::DragFloat("Intensity", &l->intensity, 0.1f);

        // Işığın etki mesafesi (Range) - Noktasal ve Spot ışıklar için kritik parametre
        ImGui::DragFloat("Range", &l->range, 1.0f);

        // 2. SPOT IŞIK ÖZEL AYARLARI (CONE ANGLES)
        // Sadece spot ışık türünde anlamlı olan iç ve dış koni açı sınırları (0-90 derece arası)
        ImGui::DragFloat("Inner Cone", &l->inner_cone, 0.5f, 0.0f, 89.0f);
        ImGui::DragFloat("Outer Cone", &l->outer_cone, 0.5f, 0.0f, 90.0f);

        // 3. IŞIK TÜRÜ SEÇİMİ (ENUM MANAGEMENT)
        const char* typeNames[] = { "Directional", "Point", "Spot" };
        int typeIndex = (int)l->type;

        // Kullanıcı Combo Box üzerinden türü değiştirdiğinde enum cast edilerek doğrudan bileşene yazılır
        if (ImGui::Combo("Type", &typeIndex, typeNames, 3))
            l->type = (light_type)typeIndex;
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse varsayılan bir ışık bileşeni ekler.
 * * @param e Işık bileşeninin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_light_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    // Varlığın zaten bir ışık bileşeni olup olmadığını kontrol ediyoruz (Mükerrerlik koruması)
    if (!reg.has_component<light_component>(e)) {
        // Varsayılan kurucu fonksiyon değerleriyle (Örn: Point Light, Beyaz Renk) bileşeni ekliyoruz
        reg.add_component<light_component>(e);
    }
}

/**
 * @brief Işık (Light) arayüz alt sistemini merkezi UI kayıt yöneticisine enjekte eder.
 * * Compile-time şablon bağımlılık zincirini kırmak amacıyla runtime üzerinde type-erased sorgu lambdası üretir.
 */
inline void register_light_ui_system() {
    // Merkezi arayüz kütüphanesine aktarılacak meta yapı
    ComponentUIInfo info;

    info.name = "Light";
    info.render_fn = render_light_ui;
    info.add_fn = add_light_component;

    // Tip gizleme (type-erasure) işlemi için has_component kontrol lambdası
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<light_component>(e);
        };

    // Light bileşenini ECS'den güvenle kaldıran remove_fn lambdası
    info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<light_component>(e);
        };

    // Benzersiz tip imzasını (hash_code) anahtar olarak kullanarak sisteme kayıt işlemini tamamlıyoruz
    component_ui_registry::register_component(typeid(light_component).hash_code(), info);
}