/**
 * @file name_component_ui.h
 * @brief İsim bileşeni (name_component) için editör kullanıcı arayüzü yönetim sistemi.
 *
 * Bu dosya, ECS mimarisindeki varlıkların (entity) editör panellerinde ve hiyerarşi
 * ağacında okunaklı isimlerle gösterilmesi, bu isimlerin doğrulanması ve merkezi UI
 * kayıt sistemine entegre edilmesini sağlar.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h" // Derleyici burada registry metotlarını net görüyor
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) isim bileşeni verilerini ImGui üzerinde çizer.
  *
  * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı (EngineContext).
  * @param editorCtx Editörün panel odakları ve arayüz durumu (EditorContext).
  */
inline void render_name_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    // Varlığa ait isim bileşeninin ham göstergesini güvenli bir şekilde çekiyoruz
    auto* n = reg.get_component<name_component>(e);
    if (n) {
        // Varlığın mevcut ismini salt okunur metin alanı olarak arayüze basıyoruz
        ImGui::Text("Name: %s", n->name.c_str());
        ImGui::Separator();
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse varsayılan bir isim bileşeni ekler.
 *
 * @param e İsim bileşeninin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_name_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    // Varlığın zaten bir isim bileşeni olup olmadığını kontrol ediyoruz (Mükerrerlik koruması)
    if (!reg.has_component<name_component>(e)) {
        // Nesneye varsayılan olarak "Entity" ismini atayarak bileşeni oluşturuyoruz
        reg.add_component<name_component>(e, name_component{ "Entity" });
    }
}

/**
 * @brief İsim (Name) arayüz alt sistemini merkezi UI kayıt yöneticisine kaydeder.
 *
 * Derleme zamanı bağımlılık zincirini yalıtmak amacıyla şablon (template) içermeyen,
 * doğrudan benzersiz tip kodu (hash_code) tabanlı kayıt algoritmasını tetikler.
 */
inline void register_name_ui_system() {
    // Merkezi arayüz kütüphanesine aktarılacak olan meta veri yapısını hazırlıyoruz
    ComponentUIInfo info;

    // Editör sisteminde kullanılacak okunaklı başlık ismi
    info.name = "Name";

    // Çizim ve ekleme fonksiyon göstergelerinin atanması
    info.render_fn = render_name_ui;
    info.add_fn = add_name_component;

    // Tip gizleme (type-erasure) işlemi için has_component kontrol lambdası
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<name_component>(e);
        };


    // Yeni non-template doğrudan kayıt fonksiyonumuzu çağırıyoruz
    // Bileşenin tip özetini (hash_code) anahtar olarak kullanarak kaydı tamamlıyoruz
    component_ui_registry::register_component(typeid(name_component).hash_code(), info);
}