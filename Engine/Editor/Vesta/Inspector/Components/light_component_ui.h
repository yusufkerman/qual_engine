#pragma once

/**
 * @file light_component_ui.h
 * @brief Işık bileşeni (light_component) için editör kullanıcı arayüzü ve operasyon kayıt sistemi.
 */

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın ışık parametrelerini ImGui üzerinde çizer.
  * @param e Işık bileşeni düzenlenecek varlık.
  * @param reg İlgili ECS kayıt defteri referansı.
  */
inline void render_light_ui(entity e, registry& reg) {
    auto* l = reg.get_component<light_component>(e);
    if (!l) return;

    // 1. Renk ve Yoğunluk Ayarları
    ImGui::ColorEdit3("Color", &l->color.x);
    ImGui::DragFloat("Intensity", &l->intensity, 0.1f);
    ImGui::DragFloat("Range", &l->range, 1.0f);

    // 2. Spot Işık Özel Ayarları (Koni Açıları)
    if (l->type == light_type::spot) {
        ImGui::DragFloat("Inner Cone", &l->inner_cone, 0.5f, 0.0f, 89.0f);
        ImGui::DragFloat("Outer Cone", &l->outer_cone, 0.5f, 0.0f, 90.0f);
    }

    // 3. Işık Türü Seçimi
    const char* typeNames[] = { "Directional", "Point", "Spot" };
    int typeIndex = static_cast<int>(l->type);

    if (ImGui::Combo("Type", &typeIndex, typeNames, 3)) {
        l->type = static_cast<light_type>(typeIndex);
    }
}

/**
 * @brief Işık bileşeni için arayüz ve operasyon alt sistemlerini merkezi Vesta yöneticilerine kaydeder.
 */
inline void register_light_ui_system() {
    size_t comp_hash = typeid(light_component).hash_code();

    // 1. UI Çizim Kaydı
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Light";
    ui_info.render_fn = render_light_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    // 2. ECS Operasyon Kaydı
    vesta::ComponentOperationInfo op_info;

    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<light_component>(e)) {
            reg.add_component<light_component>(e);
        }
        };

    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<light_component>(e);
        };

    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<light_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, op_info);
}