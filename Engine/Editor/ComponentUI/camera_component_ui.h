#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include <imgui.h>

inline void render_camera_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* c = reg.get_component<camera_component>(e);
        if (c) {
            ImGui::DragFloat("FOV", &c->fov, 1.0f);
            ImGui::DragFloat("Near", &c->nearPlane, 0.01f);
            ImGui::DragFloat("Far", &c->farPlane, 1.0f);
        }
    }
}

inline void add_camera_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<camera_component>(e)) {
        reg.add_component<camera_component>(e);
    }
}

inline void register_camera_ui_system() {
    ComponentUIInfo info;
    info.name = "Camera";
    info.render_fn = render_camera_ui;
    info.add_fn = add_camera_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<camera_component>(e);
        };

    component_ui_registry::register_component(typeid(camera_component).hash_code(), info);
}