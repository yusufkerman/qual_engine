#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

inline void render_light_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* l = reg.get_component<light_component>(e);
        if (l) {
            ImGui::ColorEdit3("Color", &l->color.x);
            ImGui::DragFloat("Intensity", &l->intensity, 0.1f);
            ImGui::DragFloat("Range", &l->range, 1.0f);
            ImGui::DragFloat("Inner Cone", &l->inner_cone, 0.5f, 0.0f, 89.0f);
            ImGui::DragFloat("Outer Cone", &l->outer_cone, 0.5f, 0.0f, 90.0f);

            const char* typeNames[] = { "Directional", "Point", "Spot" };
            int typeIndex = (int)l->type;
            if (ImGui::Combo("Type", &typeIndex, typeNames, 3))
                l->type = (light_type)typeIndex;
        }
    }
}

inline void add_light_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<light_component>(e)) {
        reg.add_component<light_component>(e);
    }
}

inline void register_light_ui_system() {
    ComponentUIInfo info;
    info.name = "Light";
    info.render_fn = render_light_ui;
    info.add_fn = add_light_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<light_component>(e);
        };

    component_ui_registry::register_component(typeid(light_component).hash_code(), info);
}