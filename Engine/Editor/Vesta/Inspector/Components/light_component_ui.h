/** * @file light_component_ui.h
 * @brief Işık bileşeni (light_component) için editör kullanıcı arayüzü yönetim sistemi.
 */
#pragma once

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

 // NOT: Artık ne engine_context.h ne de editor_context.h bağımlılıklarına ihtiyaç var!

 /** * @brief Seçili varlığın (entity) ışık bileşeni verilerini ImGui üzerinde çizip günceller.
  */
inline void render_light_ui(entity e, registry& reg) {
    auto* l = reg.get_component<light_component>(e);
    if (!l) return;

    // 1. RENK VE YOĞUNLUK AYARLARI
    ImGui::ColorEdit3("Color", &l->color.x);
    ImGui::DragFloat("Intensity", &l->intensity, 0.1f);
    ImGui::DragFloat("Range", &l->range, 1.0f);

    // 2. SPOT IŞIK ÖZEL AYARLARI (CONE ANGLES)
    if (l->type == light_type::spot) { // Opsiyonel: Sadece spot seçiliyken görünmesi daha şık olur
        ImGui::DragFloat("Inner Cone", &l->inner_cone, 0.5f, 0.0f, 89.0f);
        ImGui::DragFloat("Outer Cone", &l->outer_cone, 0.5f, 0.0f, 90.0f);
    }

    // 3. IŞIK TÜRÜ SEÇİMİ (ENUM MANAGEMENT)
    const char* typeNames[] = { "Directional", "Point", "Spot" };
    int typeIndex = (int)l->type;

    if (ImGui::Combo("Type", &typeIndex, typeNames, 3)) {
        l->type = (light_type)typeIndex;
    }
}

/** * @brief Işık (Light) arayüz ve operasyon alt sistemlerini merkezi Vesta kayıt yöneticilerine enjekte eder.
 */
inline void register_light_ui_system() {
    size_t comp_hash = typeid(light_component).hash_code();

    //----------------------------------------------------------------------
    // 1. KANAL: UI ÇİZİM KAYDI
    //----------------------------------------------------------------------
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Light";
    ui_info.render_fn = render_light_ui; // Doğrudan yeni saf fonksiyonu bağlıyoruz

    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    //----------------------------------------------------------------------
    // 2. KANAL: ECS OPERASYON KAYDI
    //----------------------------------------------------------------------
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