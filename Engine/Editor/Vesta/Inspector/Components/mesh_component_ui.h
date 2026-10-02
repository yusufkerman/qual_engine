#pragma once

/**
 * @file mesh_component_ui.h
 * @brief Mesh bileşeni (mesh_component) için model sürükle-bırak ve primitif seçimi destekli editör arayüzü.
 */

#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Services/asset_entry.h"
#include <imgui.h>
#include <iostream>

 /**
  * @brief Seçili varlığın mesh kaynağını (Model dosyası veya Primitif) ImGui üzerinde çizer ve günceller.
  * @param e Mesh verisi düzenlenecek varlık.
  * @param reg İlgili ECS kayıt defteri referansı.
  * @param ariaSys Model ve varlık yükleme işlemleri için Aria çekirdek sistemi referansı.
  */
inline void render_mesh_ui(entity e, registry& reg, aria::system& ariaSys) {
    auto* meshComp = reg.get_component<mesh_component>(e);
    if (!meshComp) return;

    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.1f, 1.0f), "Mesh Component");
    ImGui::Separator();

    // 1. Mesh Kaynağı Seçimi (Model / Primitifler)
    const char* mesh_types[] = { "Model (Asset)", "Cube (Primitive)", "Sphere (Primitive)" };
    int current_type_idx = static_cast<int>(meshComp->source);

    ImGui::Text("Mesh Source:");
    if (ImGui::Combo("##MeshTypeCombo", &current_type_idx, mesh_types, IM_ARRAYSIZE(mesh_types))) {
        meshComp->source = static_cast<mesh_source_type>(current_type_idx);

        if (meshComp->source == mesh_source_type::primitive_cube) {
            meshComp->model_handle = asset_pipeline::create_cube_mesh(&ariaSys);
            meshComp->sub_mesh_index = 0;
        }
        else if (meshComp->source == mesh_source_type::primitive_sphere) {
            meshComp->model_handle = asset_pipeline::create_sphere_mesh(&ariaSys);
            meshComp->sub_mesh_index = 0;
        }
        else if (meshComp->source == mesh_source_type::model) {
            meshComp->model_handle = aria::handle<aria::model_asset>();
        }

        // Çevreleyen hacim (Bounds) bileşenini doğrula ve güncelle
        if (!reg.has_component<bounds_component>(e)) {
            reg.add_component<bounds_component>(e);
        }
        auto* bounds = reg.get_component<bounds_component>(e);
        bounds->localCenter = glm::vec3(0.0f);
        bounds->localRadius = 1.0f;
    }

    ImGui::Spacing();

    // 2. Seçilen Kaynak Tipine Göre Alanların Çizilmesi
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
                    aria::handle<aria::model_asset> importedHandle = asset_pipeline::import_model(&ariaSys, data->path);

                    if (importedHandle.is_valid()) {
                        meshComp->model_handle = importedHandle;

                        if (!reg.has_component<bounds_component>(e)) {
                            reg.add_component<bounds_component>(e);
                        }

                        auto* bounds = reg.get_component<bounds_component>(e);
                        bounds->localCenter = glm::vec3(0.0f);
                        bounds->localRadius = 1.0f;
                    }
                    else {
                        std::cerr << "[Vesta MeshUI] Model yuklenemedi: " << data->path << "\n";
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
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Using Built-in Engine Sphere Mesh");
    }
}

/**
 * @brief Varlığa mesh_component ve bağlı bağımlılıkları (material_component) ekler.
 */
inline void add_mesh_component(entity e, registry& reg) {
    if (!reg.has_component<mesh_component>(e)) {
        reg.add_component<mesh_component>(e);
    }
    // Render sisteminin materyalsiz çizim hatası vermemesi için tamamlayıcı olarak eklenir
    if (!reg.has_component<material_component>(e)) {
        reg.add_component<material_component>(e);
    }
}

/**
 * @brief Mesh bileşeni için UI çizici ve ECS operasyon rutinlerini kaydeder.
 */
inline void register_mesh_ui_system(aria::system& ariaSys) {
    size_t comp_hash = typeid(mesh_component).hash_code();

    // 1. ECS Operasyon Kaydı
    vesta::ComponentOperationInfo logic;
    logic.add_fn = add_mesh_component;

    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<mesh_component>(e);
        };

    logic.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<mesh_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, logic);

    // 2. UI Çizim Kaydı (Aria sistemi referansı closure içine yakalanır)
    vesta::ComponentUIInfo ui;
    ui.display_name = "Mesh Renderer";
    ui.render_fn = [&ariaSys](entity e, registry& reg) {
        render_mesh_ui(e, reg, ariaSys);
        };

    vesta::component_ui_registry::register_component(comp_hash, ui);
}