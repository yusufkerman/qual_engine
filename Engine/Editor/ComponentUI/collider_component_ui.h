/**
 * @file collider_component_ui.h
 * @brief Çarpıştırıcı bileşeni (collider_component) için Jolt Physics tabanlı editör arayüzü.
 * * Bu dosya, nesnelerin fiziksel sınırlarını (Box, Sphere vb.) ImGui panellerinde
 * dinamik olarak manipüle etmeyi, doğrulamayı ve merkezi UI sistemine entegrasyonunu sağlar.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) çarpıştırıcı verilerini (tip, boyut) ImGui üzerinde çizip günceller.
  * * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı (EngineContext).
  * @param editorCtx Editörün panel ve arayüz durumu (EditorContext).
  */
inline void render_collider_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    // Varlığa ait collider bileşeninin ham göstergesini güvenli bir şekilde çekiyoruz
    auto* col = reg.get_component<collider_component>(e);
    if (!col) return;

    // ---------------------------------------------------------
    // 1. COLLIDER TIPI SEÇIMI (BOX / SPHERE)
    // ---------------------------------------------------------
    const char* colliderNames[] = { "Box", "Sphere" };
    int typeIndex = (int)col->type;

    // Kullanıcı arayüzünden tipi değiştirdiğinde enum değerini güncelliyoruz
    if (ImGui::Combo("Collider Type", &typeIndex, colliderNames, 2)) {
        col->type = (collider_type)typeIndex;
    }

    ImGui::Separator();

    // ---------------------------------------------------------
    // 2. TİPE GÖRE DİNAMİK BOYUTLANDIRMA KONTROLLERİ
    // ---------------------------------------------------------
    if (col->type == collider_type::box) {
        // Jolt Physics, kutu boyutları için "Half-Extents" (Merkezden kenara olan mesafe) bekler.
        // Kullanıcıyı bilgilendirmek adına gri renkte bir açıklama metni basıyoruz.
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Box Dimensions (Full Size)");

        glm::vec3 displayedSize = col->extents * 2.0f;

        // Kullanıcı arayüzünde tam boyutu manipüle ediyor (Adım değerini 0.1 yaptık daha rahat olsun diye)
        if (ImGui::DragFloat3("Size", &displayedSize.x, 0.1f, 0.01f, 100.0f, "%.2f")) {
            // Kullanıcı değiştirdiği an, yeni tam boyutun yarısını alıp arkadaki half-extents'e yazıyoruz
            col->extents = displayedSize * 0.5f;
        }
    }
    else if (col->type == collider_type::sphere) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Sphere Dimensions");

        // Küre geometrisi için sadece yarıçap (Radius) bilgisini çekiyoruz (X bileşenini scale kabul ederek)
        ImGui::DragFloat("Radius", &col->extents.x, 0.05f, 0.01f, 100.0f, "%.2f");

        // Veri bütünlüğünü korumak adına Y ve Z eksenlerini de X (Radius) ile senkronize tutuyoruz
        col->extents.y = col->extents.x;
        col->extents.z = col->extents.x;
    }

    ImGui::Separator();

    // ---------------------------------------------------------
    // 3. BİLEŞENİ KALDIRMA (REMOVE)
    // ---------------------------------------------------------
    if (ImGui::Button("Remove Collider")) {
        // Varlığı ECS dünyasındaki collider kütüphanesinden güvenle siliyoruz
        reg.remove_component<collider_component>(e);
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse varsayılan bir collider bileşeni ekler.
 * * @param e Bileşenin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_collider_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    // Varlığın zaten bir collider'ı olup olmadığını kontrol ediyoruz (Mükerrerlik koruması)
    if (!reg.has_component<collider_component>(e)) {
        // Orijinal motor standartlarına sadık kalarak: Varsayılan olarak Box tipi ve 1x1x1 boyutlarında başlatıyoruz
        reg.add_component<collider_component>(e, collider_component{ collider_type::box, glm::vec3(1.0f) });
    }
}

/**
 * @brief Çarpıştırıcı (Collider) arayüz alt sistemini merkezi kayıt yöneticisine kaydeder.
 * * Şablon mimarisini runtime üzerinde çözümlemek adına type-erased lambdalar üretir.
 */
inline void register_collider_ui_system() {
    // Kayıt veri yapısını hazırlıyoruz
    ComponentUIInfo info;

    info.name = "Collider";
    info.render_fn = render_collider_ui;
    info.add_fn = add_collider_component;

    // Tip gizleme (type-erasure) işlemi için has_component kontrol lambdası
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<collider_component>(e);
        };

    // Collider bileşenini ECS'den güvenle kaldıran remove_fn lambdası
    info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<collider_component>(e);
        };

    // Benzersiz hash kodunu kullanarak bileşeni merkezi UI kütüphanesine ekliyoruz
    component_ui_registry::register_component(typeid(collider_component).hash_code(), info);
}