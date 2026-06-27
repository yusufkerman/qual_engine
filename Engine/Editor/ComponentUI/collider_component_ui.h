#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>

inline void render_collider_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    if (ImGui::CollapsingHeader("Collider (Jolt Shape)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* col = reg.get_component<collider_component>(e);
        if (!col) return;

        // 1. Collider Tipi Seçimi
        const char* colliderNames[] = { "Box", "Sphere" };
        int typeIndex = (int)col->type;

        if (ImGui::Combo("Collider Type", &typeIndex, colliderNames, 2)) {
            col->type = (collider_type)typeIndex;
        }

        ImGui::Separator();

        // 2. Boyutlandırma Kontrolleri
        if (col->type == collider_type::box) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Box Dimensions (Half-Extents)");
            ImGui::DragFloat3("Extents", &col->extents.x, 0.05f, 0.01f, 100.0f, "%.2f");
        }
        else if (col->type == collider_type::sphere) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Sphere Dimensions");
            ImGui::DragFloat("Radius", &col->extents.x, 0.05f, 0.01f, 100.0f, "%.2f");

            col->extents.y = col->extents.x;
            col->extents.z = col->extents.x;
        }

        ImGui::Separator();

        if (ImGui::Button("Remove Collider")) {
            reg.remove_component<collider_component>(e);
        }
    }
}

inline void add_collider_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<collider_component>(e)) {
        // Orijinal kodundaki varsayılan değerler (Box ve 1x1x1)
        reg.add_component<collider_component>(e, collider_component{ collider_type::box, glm::vec3(1.0f) });
    }
}

inline void register_collider_ui_system() {
    ComponentUIInfo info;
    info.name = "Collider";
    info.render_fn = render_collider_ui;
    info.add_fn = add_collider_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<collider_component>(e);
        };

    component_ui_registry::register_component(typeid(collider_component).hash_code(), info);
}