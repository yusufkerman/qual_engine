#pragma once

/**
 * @file collider_component_ui.h
 * @brief Çarpıştırıcı bileşeni (collider_component) için editör arayüzü ve operasyon kayıt sistemi.
 */

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın çarpıştırıcı sınırlarını ve türünü ImGui üzerinde çizer.
  * @param e Çarpıştırıcı verisi görüntülenecek varlık.
  * @param reg İlgili ECS kayıt defteri referansı.
  */
inline void render_collider_ui(entity e, registry& reg) {
    auto* col = reg.get_component<collider_component>(e);
    if (!col) return;

    // 1. Çarpıştırıcı Türü Seçimi (Box / Sphere)
    const char* colliderNames[] = { "Box", "Sphere" };
    int typeIndex = static_cast<int>(col->type);

    if (ImGui::Combo("Collider Type", &typeIndex, colliderNames, 2)) {
        col->type = static_cast<collider_type>(typeIndex);
        col->extents = glm::vec3(0.5f); // Tür değişiminde varsayılan boyuta resetle
    }

    ImGui::Separator();

    // 2. Türe Göre Dinamik Boyut Kontrolleri
    if (col->type == collider_type::box) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Box Dimensions (Full Size)");

        // Fizik motoru half-extents beklediği için kullanıcıya tam boyut (Full Size) gösterilir
        glm::vec3 displayedSize = col->extents * 2.0f;

        if (ImGui::DragFloat3("Size", &displayedSize.x, 0.1f, 0.01f, 100.0f, "%.2f")) {
            col->extents = displayedSize * 0.5f;
        }
    }
    else if (col->type == collider_type::sphere) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Sphere Dimensions");

        // Küre için yarıçap (Radius) manipülasyonu
        if (ImGui::DragFloat("Radius", &col->extents.x, 0.05f, 0.01f, 100.0f, "%.2f")) {
            // Eksen senkronizasyonunu koru
            col->extents.y = col->extents.x;
            col->extents.z = col->extents.x;
        }
    }
}

/**
 * @brief Çarpıştırıcı bileşeni için UI çizici ve ECS operasyon rutinlerini sisteme kaydeder.
 */
inline void register_collider_ui_system() {
    size_t comp_hash = typeid(collider_component).hash_code();

    // 1. UI Çizim Kaydı
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Collider";
    ui_info.category = "Physics";
    ui_info.render_fn = render_collider_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    // 2. ECS Operasyon Kaydı
    vesta::ComponentOperationInfo op_info;

    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<collider_component>(e)) {
            // Varsayılan olarak 1x1x1 tam boyutta (0.5f half-extents) Box başlatılır
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