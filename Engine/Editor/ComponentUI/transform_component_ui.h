#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h" // <-- Derleyici burada registry'nin tüm metotlarını görüyor!
#include <imgui.h>

inline void render_transform_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* t = reg.get_component<transform_component>(e);
        if (t) {
            ImGui::DragFloat3("Position", &t->position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);
            ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
        }
    }
}

inline void add_transform_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<transform_component>(e)) {
        reg.add_component<transform_component>(e);
    }
}

inline void register_transform_ui_system() {
    ComponentUIInfo info;
    info.name = "Transform";
    info.render_fn = render_transform_ui;
    info.add_fn = add_transform_component;

    // Lambdayı burada tanımlıyoruz, derleyici reg.has_component'ı başarıyla çözüyor!
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<transform_component>(e);
        };

    component_ui_registry::register_component(typeid(transform_component).hash_code(), info);
}