#pragma once

/**
 * @file rigidbody_component_ui.h
 * @brief Katı cisim bileşeni (rigidbody_component) için fizik parametrelerini yöneten editör arayüzü.
 */

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın hareket türü, kütle ve fizik kısıtlamalarını ImGui üzerinde çizer.
  * @param e Katı cisim parametreleri düzenlenecek varlık.
  * @param reg İlgili ECS kayıt defteri referansı.
  */
inline void render_rigidbody_ui(entity e, registry& reg) {
    auto* rb = reg.get_component<rigidbody_component>(e);
    if (!rb) return;

    // Fizik Durumu Kontrolü
    if (!rb->body_id.IsInvalid()) {
        ImGui::Text("Physics State: Active (Simulation)");
    }
    else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Editor Mode (Physics Idle)");
    }

    ImGui::Separator();

    // 1. Hareket Türü (Static / Kinematic / Dynamic)
    const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };
    int motionIndex = static_cast<int>(rb->motion);

    if (ImGui::Combo("Motion Type", &motionIndex, motionNames, 3)) {
        rb->motion = static_cast<motion_type>(motionIndex);
    }

    // 2. Kütle, Sürtünme ve Esneklik Ayarları
    if (rb->motion == motion_type::static_body) {
        ImGui::BeginDisabled();
        float dummyMass = 0.0f;
        ImGui::DragFloat("Mass (Kg)", &dummyMass, 0.0f);
        ImGui::EndDisabled();
    }
    else {
        ImGui::DragFloat("Mass (Kg)", &rb->mass, 0.05f, 0.01f, 10000.0f, "%.2f");
    }

    ImGui::SliderFloat("Friction", &rb->friction, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Restitution", &rb->restitution, 0.0f, 1.0f, "%.2f");

    ImGui::Separator();

    // 3. Eksenel Rotasyon Kısıtlamaları (Freeze Rotation)
    ImGui::Text("Constraints");
    ImGui::Text("Freeze Rotation:");
    ImGui::SameLine();

    ImGui::Checkbox("X", &rb->freeze_rotation_x); ImGui::SameLine();
    ImGui::Checkbox("Y", &rb->freeze_rotation_y); ImGui::SameLine();
    ImGui::Checkbox("Z", &rb->freeze_rotation_z);
}

/**
 * @brief Katı cisim bileşeni için arayüz ve ECS operasyon sistemlerini kaydeder.
 */
inline void register_rigidbody_ui_system() {
    size_t comp_hash = typeid(rigidbody_component).hash_code();

    // 1. UI Çizim Kaydı
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Rigidbody";
    ui_info.render_fn = render_rigidbody_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    // 2. ECS Operasyon Kaydı
    vesta::ComponentOperationInfo op_info;

    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<rigidbody_component>(e)) {
            reg.add_component<rigidbody_component>(e, rigidbody_component{ motion_type::dynamic_body, 1.0f, 0.5f });
        }
        };

    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<rigidbody_component>(e);
        };

    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<rigidbody_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, op_info);
}