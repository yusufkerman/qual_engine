#pragma once

/**
 * @file bone_attachment_component_ui.h
 * @brief Kemik eklentisi/soket bileşeni (bone_attachment_component) için editör arayüzü.
 */

#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include "../../ECS/components.h"
#include <imgui.h>
#include <string>

inline void render_bone_attachment_ui(entity e, registry& reg) {
    auto* attach = reg.get_component<bone_attachment_component>(e);
    if (!attach) return;

    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.2f, 1.0f), "Bone Attachment (Socket)");
    ImGui::Separator();

    // Hedef Skeleton Entity
    int targetEntityId = static_cast<int>(attach->target_skeleton_entity);
    if (ImGui::InputInt("Target Skeleton Entity", &targetEntityId)) {
        if (targetEntityId < 0) targetEntityId = 0;
        attach->target_skeleton_entity = static_cast<entity>(targetEntityId);
        attach->cached_bone_index = -1; // Kemik önbelleğini sıfırla
    }

    // Kemik Adı
    char boneNameBuf[128];
    strncpy_s(boneNameBuf, sizeof(boneNameBuf), attach->bone_name.c_str(), sizeof(boneNameBuf) - 1);
    if (ImGui::InputText("Bone Name", boneNameBuf, sizeof(boneNameBuf))) {
        attach->bone_name = boneNameBuf;
        attach->cached_bone_index = -1;
    }

    if (attach->cached_bone_index >= 0) {
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Resolved Bone Index: %d", attach->cached_bone_index);
    } else if (!attach->bone_name.empty()) {
        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "Bone Index Not Yet Resolved");
    }

    ImGui::Spacing();

    // Yerel Sapmalar (Offset, Rotation, Scale)
    ImGui::DragFloat3("Local Offset", &attach->local_offset.x, 0.05f);
    ImGui::DragFloat3("Local Rotation", &attach->local_rotation.x, 1.0f);
    ImGui::DragFloat3("Local Scale", &attach->local_scale.x, 0.05f, 0.001f, 100.0f);
}

inline void register_bone_attachment_ui_system() {
    size_t comp_hash = typeid(bone_attachment_component).hash_code();

    vesta::ComponentOperationInfo logic;
    logic.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<bone_attachment_component>(e)) {
            reg.add_component<bone_attachment_component>(e);
        }
    };
    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<bone_attachment_component>(e);
    };
    logic.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<bone_attachment_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, logic);

    vesta::ComponentUIInfo ui;
    ui.display_name = "Bone Attachment";
    ui.render_fn = [](entity e, registry& reg) {
        render_bone_attachment_ui(e, reg);
    };
    vesta::component_ui_registry::register_component(comp_hash, ui);
}

