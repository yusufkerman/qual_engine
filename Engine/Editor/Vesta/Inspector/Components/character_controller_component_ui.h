#pragma once

/**
 * @file character_controller_component_ui.h
 * @brief Kinematik karakter kontrolcusu (character_controller_component) icin editor arayuzu ve kayit sistemi.
 */

#include "../Vesta/Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/components.h"
#include "../../ECS/registry.h"
#include <imgui.h>

/**
 * @brief Secili varligin Character Controller parametrelerini ImGui uzerinde cizer.
 */
inline void render_character_controller_ui(entity e, registry& reg) {
    auto* cc = reg.get_component<character_controller_component>(e);
    if (!cc) return;

    if (cc->character) {
        ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.52f, 1.0f), "Jolt CharacterVirtual: Active");
        ImGui::Text("Grounded: %s", cc->is_grounded ? "Yes" : "No");
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", cc->linear_velocity.x, cc->linear_velocity.y, cc->linear_velocity.z);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Idle (Instantiates on Play)");
    }

    ImGui::Separator();

    ImGui::TextDisabled("Capsule Dimensions");
    ImGui::DragFloat("Radius", &cc->radius, 0.01f, 0.05f, 5.0f, "%.2f m");
    ImGui::DragFloat("Height", &cc->height, 0.02f, 0.1f, 10.0f, "%.2f m");

    ImGui::Separator();

    ImGui::TextDisabled("Locomotion");
    ImGui::DragFloat("Walk Speed", &cc->walk_speed, 0.1f, 0.1f, 50.0f, "%.1f m/s");
    ImGui::DragFloat("Jump Speed", &cc->jump_speed, 0.1f, 0.1f, 50.0f, "%.1f m/s");
    ImGui::DragFloat("Gravity", &cc->gravity, 0.1f, -50.0f, 0.0f, "%.2f m/s²");

    ImGui::Separator();

    ImGui::TextDisabled("Environment Traversal");
    ImGui::DragFloat("Step Height", &cc->step_height, 0.01f, 0.0f, 2.0f, "%.2f m");
    ImGui::SliderFloat("Max Slope Angle", &cc->max_slope_angle, 0.0f, 89.0f, "%.1f deg");
}

/**
 * @brief Character Controller bileseni icin UI ve ECS operasyon kayitlarini gerceklestirir.
 */
inline void register_character_controller_ui_system() {
    size_t comp_hash = typeid(character_controller_component).hash_code();

    // 1. UI Cizim Kaydi
    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Character Controller";
    ui_info.category = "Physics";
    ui_info.render_fn = render_character_controller_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    // 2. ECS Operasyon Kaydi
    vesta::ComponentOperationInfo op_info;

    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<character_controller_component>(e)) {
            reg.add_component<character_controller_component>(e);
        }
    };

    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<character_controller_component>(e);
    };

    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<character_controller_component>(e);
    };

    vesta::component_operation_registry::register_component(comp_hash, op_info);
}

