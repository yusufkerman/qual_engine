#pragma once

/**
 * @file camera_component_ui.h
 * @brief Kamera bileşeni (camera_component) için editör arayüzü ve ECS operasyon kayıt sistemi.
 */

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) kamera bileşeni verilerini ImGui üzerinde çizer.
  * @param e Kamera bileşeni çizilecek varlık.
  * @param reg İlgili ECS kayıt defteri referansı.
  */
inline void render_camera_ui(entity e, registry& reg) {
    auto* c = reg.get_component<camera_component>(e);
    if (!c) return;

    // Görüş Açısı (Field of View)
    ImGui::DragFloat("FOV", &c->fov, 1.0f, 1.0f, 179.0f, "%.0f°");

    // Yakın Kırpma Düzlemi (Near Clip Plane)
    ImGui::DragFloat("Near", &c->nearPlane, 0.01f, 0.001f, 10.0f, "%.3f");

    // Uzak Kırpma Düzlemi (Far Clip Plane)
    ImGui::DragFloat("Far", &c->farPlane, 1.0f, 10.0f, 10000.0f, "%.0f");
}

/**
 * @brief Kamera bileşeni için UI çizici ve ECS yönetim rutinlerini kaydeder.
 */
inline void register_camera_ui_system() {
    size_t comp_hash = typeid(camera_component).hash_code();

    // 1. UI Çizim Kaydı
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Camera";
    ui_info.render_fn = render_camera_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    // 2. ECS Operasyon Kaydı
    vesta::ComponentOperationInfo op_info;

    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<camera_component>(e)) {
            reg.add_component<camera_component>(e);
        }
        };

    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<camera_component>(e);
        };

    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<camera_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, op_info);
}