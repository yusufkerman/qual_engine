/** * @file collider_component_ui.h * @brief Çarpıştırıcı bileşeni (collider_component) için Jolt Physics tabanlı editör arayüzü. */
#pragma once

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

// NOT: Artık ne engine_context.h ne de dış fizik bağımlılıklarına ihtiyaç var!

/** * @brief Seçili varlığın (entity) çarpıştırıcı verilerini ImGui üzerinde çizip günceller.  */
inline void render_collider_ui(entity e, registry& reg) {
    auto* col = reg.get_component<collider_component>(e);
    if (!col) return;

    // ---------------------------------------------------------
    // 1. COLLIDER TIPI SEÇIMI (BOX / SPHERE)
    // ---------------------------------------------------------
    const char* colliderNames[] = { "Box", "Sphere" };
    int typeIndex = (int)col->type;

    if (ImGui::Combo("Collider Type", &typeIndex, colliderNames, 2)) {
        col->type = (collider_type)typeIndex;
        // 💡 Taktik: Tip değiştiğinde boyut saçmalamasın diye varsayılan bir scale'e çekebilirsin:
        col->extents = glm::vec3(0.5f);
    }

    ImGui::Separator();

    // ---------------------------------------------------------
    // 2. TİPE GÖRE DİNAMİK BOYUTLANDIRMA KONTROLLERİ
    // ---------------------------------------------------------
    if (col->type == collider_type::box) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Box Dimensions (Full Size)");

        // Jolt half-extents beklediği için kullanıcıya tam boyutu (Full Size) gösteriyoruz
        glm::vec3 displayedSize = col->extents * 2.0f;

        if (ImGui::DragFloat3("Size", &displayedSize.x, 0.1f, 0.01f, 100.0f, "%.2f")) {
            // Değişiklik anında yarısını alıp arkadaki half-extents'e yazıyoruz
            col->extents = displayedSize * 0.5f;
        }
    }
    else if (col->type == collider_type::sphere) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Sphere Dimensions");

        // Küre için yarıçap (Radius) manipülasyonu
        ImGui::DragFloat("Radius", &col->extents.x, 0.05f, 0.01f, 100.0f, "%.2f");

        // Veri bütünlüğü için tüm eksenleri senkronize tutuyoruz
        col->extents.y = col->extents.x;
        col->extents.z = col->extents.x;
    }

    // 🔥 DİKKAT: "Remove Collider" butonu buradan TAMAMEN KALDIRILDI!
    // Vesta'nın inspector_panel'i zaten tepede jenerik bir kapatma (X veya Remove) butonu sunuyor.
}

inline void register_collider_ui_system() {
    size_t comp_hash = typeid(collider_component).hash_code();

    //----------------------------------------------------------------------
    // 1. KANAL: VESTA UI ÇİZİM KAYDI
    //----------------------------------------------------------------------
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Collider";
    ui_info.render_fn = render_collider_ui; // Doğrudan parametre arındırılmış fonksiyon

    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    //----------------------------------------------------------------------
    // 2. KANAL: ECS OPERASYON KAYDI
    //----------------------------------------------------------------------
    vesta::ComponentOperationInfo op_info;

    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<collider_component>(e)) {
            // Varsayılan olarak 1x1x1 tam boyutta (yani 0.5f half-extents) Box başlatıyoruz
            reg.add_component<collider_component>(e, collider_component{ collider_type::box, glm::vec3(0.5f) });
        }
        };

    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<collider_component>(e);
        };

    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<collider_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, op_info);
}