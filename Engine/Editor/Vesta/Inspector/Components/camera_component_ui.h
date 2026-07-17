/** * @file camera_component_ui.h * @brief Kamera bileşeni (camera_component) için editör arayüzü yönetim sistemi. */
#pragma once

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

// NOT: Artık ne engine_context.h ne de başka bir dış bağımlılığa ihtiyaç var!

/** * @brief Seçili varlığın (entity) kamera bileşeni verilerini ImGui üzerinde çizip günceller.  */
inline void render_camera_ui(entity e, registry& reg) {
    auto* c = reg.get_component<camera_component>(e);
    if (!c) return;

    // Görüş Açısı (Field of View) ayarı
    ImGui::DragFloat("FOV", &c->fov, 1.0f, 1.0f, 179.0f, "%.0f°");

    // Yakın Kırpma Düzlemi (Near Clip Plane)
    ImGui::DragFloat("Near", &c->nearPlane, 0.01f, 0.001f, 10.0f, "%.3f");

    // Uzak Kırpma Düzlemi (Far Clip Plane)
    ImGui::DragFloat("Far", &c->farPlane, 1.0f, 10.0f, 10000.0f, "%.0f");
}

inline void register_camera_ui_system() {
    size_t comp_hash = typeid(camera_component).hash_code();

    //----------------------------------------------------------------------
    // 1. KANAL: VESTA UI ÇİZİM KAYDI
    //----------------------------------------------------------------------
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Camera";
    ui_info.render_fn = render_camera_ui; // Parametrelerden arınmış saf render fonksiyonu

    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    //----------------------------------------------------------------------
    // 2. KANAL: ECS OPERASYON KAYDI
    //----------------------------------------------------------------------
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
        // Kameralar arka planda harici bir kütüphaneyi tetiklemediği için 
        // doğrudan ECS üzerinden sessizce yok edilebilirler.
        reg.remove_component<camera_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, op_info);
}