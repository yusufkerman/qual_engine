#pragma once

/**
 * @file skeleton_component_ui.h
 * @brief İskelet bileşeni (skeleton_component) için kemik ağacı ve durum izleme arayüzü.
 */

#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include "../../ECS/components.h"
#include "../../Engine/asset_pipeline.h"
#include <imgui.h>

inline void render_skeleton_node_tree(const aria::skeleton_node& node, const skeleton_component& skel) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (node.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    std::string label = node.name;
    if (node.bone_index >= 0) {
        label += " [ID: " + std::to_string(node.bone_index) + "]";
    }

    bool open = ImGui::TreeNodeEx(label.c_str(), flags);
    if (open && !node.children.empty()) {
        for (const auto& child : node.children) {
            render_skeleton_node_tree(child, skel);
        }
        ImGui::TreePop();
    }
}

inline void render_skeleton_ui(entity e, registry& reg, aria::system& ariaSys) {
    auto* skel = reg.get_component<skeleton_component>(e);
    if (!skel) return;

    ImGui::TextColored(ImVec4(0.8f, 0.4f, 0.9f, 1.0f), "Skeleton Component");
    ImGui::Separator();

    if (!skel->skeleton_handle.is_valid()) {
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "No Skeleton Asset Assigned");
        return;
    }

    const auto* skelAsset = ariaSys.get_skeleton_data(skel->skeleton_handle);
    if (!skelAsset) {
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Skeleton Asset Not Found");
        return;
    }

    ImGui::Text("Total Bones: %zu", skelAsset->raw_data.bones.size());
    ImGui::Text("Skinning Palette Size: %zu", skel->skinning_palette.size());

    // Kemik Hiyerarşisi Ağacı (Collapsible)
    if (ImGui::CollapsingHeader("Bone Hierarchy Tree")) {
        render_skeleton_node_tree(skelAsset->raw_data.root_node, *skel);
    }
}

inline void register_skeleton_ui_system(aria::system& ariaSys) {
    size_t comp_hash = typeid(skeleton_component).hash_code();

    vesta::ComponentOperationInfo logic;
    logic.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<skeleton_component>(e)) {
            reg.add_component<skeleton_component>(e);
        }
    };
    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<skeleton_component>(e);
    };
    logic.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<skeleton_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, logic);

    vesta::ComponentUIInfo ui;
    ui.display_name = "Skeleton";
    ui.render_fn = [&ariaSys](entity e, registry& reg) {
        render_skeleton_ui(e, reg, ariaSys);
    };
    vesta::component_ui_registry::register_component(comp_hash, ui);
}

