/**
 * @file transform_component_ui.h
 * @brief Dönüşüm bileşeni (transform_component) için editör kullanıcı arayüzü yönetim sistemi.
 *
 * Bu dosya; nesnelerin dünya uzayındaki veya yerel uzaydaki konum (Position), rotasyon (Rotation)
 * ve ölçek (Scale) değerlerinin ImGui panellerinde gerçek zamanlı manipüle edilmesini ve
 * merkezi arayüz sistemine kaydedilmesini sağlar.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h" // Derleyici burada registry'nin tüm metotlarını görüyor
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) dönüşüm (transform) verilerini ImGui üzerinde çizip günceller.
  *
  * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı (EngineContext).
  * @param editorCtx Editörün panel odakları ve arayüz durumu (EditorContext).
  */
inline void render_transform_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    // Görsel bütünlük için katlanabilir bir başlık (Collapsing Header) oluşturuyoruz
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

        // Varlığa ait transform bileşeninin ham göstergesini güvenli bir şekilde çekiyoruz
        auto* t = reg.get_component<transform_component>(e);
        if (t) {
            // Konum (X, Y, Z) Ayarı - 0.1 birimlik adımlarla kaydırılabilir
            ImGui::DragFloat3("Position", &t->position.x, 0.1f);

            // Rotasyon (X, Y, Z - Euler Açıları) Ayarı - 0.1 birimlik adımlarla kaydırılabilir
            ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);

            // Ölçekleme (X, Y, Z) Ayarı - 0.1 birimlik adımlarla kaydırılabilir
            ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
        }
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse varsayılan bir transform bileşeni ekler.
 *
 * @param e Dönüşüm bileşeninin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_transform_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    // Varlığın zaten bir transform bileşeni olup olmadığını kontrol ediyoruz (Mükerrerlik koruması)
    if (!reg.has_component<transform_component>(e)) {
        // Varsayılan kurucu fonksiyon değerleriyle (Konum: 0,0,0 / Rotasyon: 0,0,0 / Ölçek: 1,1,1) bileşeni ekler
        reg.add_component<transform_component>(e);
    }
}

/**
 * @brief Dönüşüm (Transform) arayüz alt sistemini merkezi UI kayıt yöneticisine enjekte eder.
 *
 * Derleme zamanı bağımlılık zincirini yalıtmak amacıyla şablon (template) içermeyen,
 * doğrudan benzersiz tip kodu (hash_code) tabanlı kayıt algoritmasını tetikler.
 */
inline void register_transform_ui_system() {
    // Merkezi arayüz kütüphanesine aktarılacak olan meta veri yapısını hazırlıyoruz
    ComponentUIInfo info;

    // Editör sisteminde kullanılacak okunaklı başlık ismi
    info.name = "Transform";

    // Çizim ve ekleme fonksiyon göstergelerinin atanması
    info.render_fn = render_transform_ui;
    info.add_fn = add_transform_component;

    // Lambdayı burada tanımlıyoruz, derleyici reg.has_component'ı başarıyla çözüyor
    // Tip gizleme (type-erasure) işlemi için has_component kontrol lambdası
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<transform_component>(e);
        };

    // Transform bileşeninin benzersiz tip kodunu anahtar olarak kullanarak merkezi sisteme kayıt ediyoruz
    component_ui_registry::register_component(typeid(transform_component).hash_code(), info);
}