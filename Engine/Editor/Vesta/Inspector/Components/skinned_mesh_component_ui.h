#pragma once

/**
 * @file skinned_mesh_component_ui.h
 * @brief İskeletli Mesh bileşeni (skinned_mesh_component) için model seçimi ve sürükle-bırak destekli editör arayüzü.
 */

#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include "../../ECS/components.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Services/asset_entry.h"
#include <imgui.h>
#include <iostream>

inline void render_skinned_mesh_ui(entity e, registry& reg, aria::system& ariaSys) {
    auto* comp = reg.get_component<skinned_mesh_component>(e);
    if (!comp) return;

    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.9f, 1.0f), "Skinned Mesh Component");
    ImGui::Separator();

    std::string modelName = "None (Drag Skinned Model Here)";
    if (comp->model_handle.is_valid()) {
        const auto* modelData = ariaSys.get_skinned_model_data(comp->model_handle);
        if (modelData && !modelData->raw_data.debug_path.empty()) {
            modelName = modelData->raw_data.debug_path;
        } else {
            modelName = "Loaded Skinned Model Asset";
        }
    }

    ImGui::Text("Skinned Model Asset:");
    ImGui::Button(modelName.c_str(), ImVec2(-1, 35));

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
            auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);

            if (data && data->type == vesta::asset_payload_type::model) {
                aria::handle<aria::skinned_model_asset> importedHandle =
                    asset_pipeline::import_skinned_model(&ariaSys, data->path);

                if (importedHandle.is_valid()) {
                    comp->model_handle = importedHandle;

                    // Eğer varlıkta skeleton_component yoksa ve modelde iskelet varsa otomatik bağla
                    const auto* modelData = ariaSys.get_skinned_model_data(importedHandle);
                    if (modelData && modelData->skeleton_handle.is_valid()) {
                        if (!reg.has_component<skeleton_component>(e)) {
                            reg.add_component<skeleton_component>(e);
                        }
                        auto* skelComp = reg.get_component<skeleton_component>(e);
                        skelComp->skeleton_handle = modelData->skeleton_handle;
                        skelComp->is_dirty = true;
                    }

                    // Eğer modelde animasyonlar varsa ve animator_component yoksa ilk animasyonu bağla
                    if (modelData && !modelData->animation_handles.empty()) {
                        if (!reg.has_component<animator_component>(e)) {
                            reg.add_component<animator_component>(e);
                        }
                        auto* animComp = reg.get_component<animator_component>(e);
                        animComp->current_clip = modelData->animation_handles[0];
                        animComp->is_playing = true;
                        animComp->is_looping = true;
                    }

                    if (!reg.has_component<bounds_component>(e)) {
                        reg.add_component<bounds_component>(e);
                    }
                    auto* bounds = reg.get_component<bounds_component>(e);
                    bounds->localCenter = glm::vec3(0.0f);
                    bounds->localRadius = 1.0f;
                }
                else {
                    std::cerr << "[Vesta SkinnedMeshUI] Skinned model yuklenemedi: " << data->path << "\n";
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Alt Ağ (Sub-Mesh) Bilgisi
    if (comp->model_handle.is_valid()) {
        const auto* modelData = ariaSys.get_skinned_model_data(comp->model_handle);
        if (modelData) {
            ImGui::Text("Sub-Meshes: %zu (All Parts Auto-Rendered)", modelData->sub_meshes.size());
        }
    }

    // Gölge Çizimi
    ImGui::Checkbox("Cast Shadows", &comp->cast_shadows);
}

inline void add_skinned_mesh_component(entity e, registry& reg) {
    if (!reg.has_component<skinned_mesh_component>(e)) {
        reg.add_component<skinned_mesh_component>(e);
    }
    if (!reg.has_component<material_component>(e)) {
        reg.add_component<material_component>(e);
    }
}

inline void register_skinned_mesh_ui_system(aria::system& ariaSys) {
    size_t comp_hash = typeid(skinned_mesh_component).hash_code();

    vesta::ComponentOperationInfo logic;
    logic.add_fn = add_skinned_mesh_component;
    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<skinned_mesh_component>(e);
    };
    logic.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<skinned_mesh_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, logic);

    vesta::ComponentUIInfo ui;
    ui.display_name = "Skinned Mesh Renderer";
    ui.category = "Rendering";
    ui.render_fn = [&ariaSys](entity e, registry& reg) {
        render_skinned_mesh_ui(e, reg, ariaSys);
    };
    vesta::component_ui_registry::register_component(comp_hash, ui);
}

