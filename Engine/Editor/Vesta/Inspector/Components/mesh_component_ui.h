/**
 * @file mesh_component_ui.h
 * @brief Aria Handle tabanlı ve Sürükle-Bırak destekli Mesh UI Yönetimi.
 */
#pragma once
#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"

#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h" 
#include "../../Panels/asset_browser_panel.h" 
#include <imgui.h>
#include <iostream>

inline void render_mesh_ui(entity e, registry& reg, aria::system& ariaSys) {
    auto* meshComp = reg.get_component<mesh_component>(e);
    if (!meshComp) return;

    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.1f, 1.0f), "📦 Mesh Component");
    ImGui::Separator();

    // ---------------------------------------------------------
    // 🛠️ MESH TÜRÜ SEÇİMİ (COMBO BOX)
    // ---------------------------------------------------------
    const char* mesh_types[] = { "Model (Asset)", "Cube (Primitive)", "Sphere (Primitive)" };
    int current_type_idx = static_cast<int>(meshComp->source);

    ImGui::Text("Mesh Source:");
    if (ImGui::Combo("##MeshTypeCombo", &current_type_idx, mesh_types, IM_ARRAYSIZE(mesh_types))) {
        meshComp->source = static_cast<mesh_source_type>(current_type_idx);

        if (meshComp->source == mesh_source_type::primitive_cube) {
            meshComp->model_handle = asset_pipeline::create_cube_mesh(&ariaSys);
            meshComp->sub_mesh_index = 0;
            std::cout << "[UI Mesh] Entity=" << (int)e << " için Built-in Cube yuklendi.\n";
        }
        else if (meshComp->source == mesh_source_type::primitive_sphere) {
            meshComp->model_handle = asset_pipeline::create_sphere_mesh(&ariaSys);
            meshComp->sub_mesh_index = 0;
            std::cout << "[UI Mesh] Entity=" << (int)e << " için Built-in Sphere yuklendi.\n";
        }
        else if (meshComp->source == mesh_source_type::model) {
            meshComp->model_handle = aria::handle<aria::model_asset>();
        }

        // Bounds otomatik ayarla
        if (!reg.has_component<bounds_component>(e)) {
            reg.add_component<bounds_component>(e);
        }
        auto* bounds = reg.get_component<bounds_component>(e);
        bounds->localCenter = glm::vec3(0.0f);
        bounds->localRadius = 1.0f; // Primitifler için varsayılan 1.0f kalsın, scale zaten transformda halledilir
    }

    ImGui::Spacing();

    // ---------------------------------------------------------
    // 👁️ SEÇİLEN SOURCE TİPİNE GÖRE ARALARI ÇİZME
    // ---------------------------------------------------------
    if (meshComp->source == mesh_source_type::model) {
        std::string modelName = "None (Drag Model Here)";
        if (meshComp->model_handle.is_valid()) {
            modelName = "Loaded Model Asset";
        }

        ImGui::Text("Model Asset:");
        ImGui::Button(modelName.c_str(), ImVec2(-1, 35));

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);

                if (data && data->type == vesta::asset_payload_type::model) {
                    std::cout << "[UI Mesh] Model suruklendi: " << data->path << "\n";
                    aria::handle<aria::model_asset> importedHandle = asset_pipeline::import_model(&ariaSys, data->path);

                    if (importedHandle.is_valid()) {
                        meshComp->model_handle = importedHandle;

                        if (!reg.has_component<bounds_component>(e))
                            reg.add_component<bounds_component>(e);

                        auto* bounds = reg.get_component<bounds_component>(e);
                        bounds->localCenter = glm::vec3(0.0f);
                        bounds->localRadius = 1.0f;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    else if (meshComp->source == mesh_source_type::primitive_cube) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Using Built-in Engine Cube Mesh");
    }
    else if (meshComp->source == mesh_source_type::primitive_sphere) {
        // 🎯 Gereksiz SliderFloat kaldırıldı, sadece bilgilendirme metni çiziyoruz
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Using Built-in Engine Sphere Mesh");
    }
}

inline void add_mesh_component(entity e, registry& reg) {
    if (!reg.has_component<mesh_component>(e)) {
        reg.add_component<mesh_component>(e);
    }
    // Render sisteminin nesneyi çizerken hata vermemesi için kombo olarak materyali de ekliyoruz
    if (!reg.has_component<material_component>(e)) {
        reg.add_component<material_component>(e);
    }
}

inline void register_mesh_ui_system(aria::system& ariaSys) {

    // --- Mantık kısmı (component_registry): ARIA bilmez ---
    vesta::ComponentOperationInfo logic;
    logic.add_fn = add_mesh_component;

    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<mesh_component>(e);
        };

    logic.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<mesh_component>(e);
        };

    vesta::component_operation_registry::register_component(typeid(mesh_component).hash_code(), logic);

    // --- UI kısmı (component_ui_registry): ARIA'yı capture eder ---
    vesta::ComponentUIInfo ui;
    ui.display_name = "Mesh Renderer";
    ui.render_fn = [&ariaSys](entity e, registry& reg) {
        render_mesh_ui(e, reg, ariaSys);
        };

    vesta::component_ui_registry::register_component(typeid(mesh_component).hash_code(), ui);
}