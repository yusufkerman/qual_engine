#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Externals/nfd/nfd.hpp"
#include "../../Externals/nfd/nfd.h"
#include <imgui.h>
#include <iostream>

inline void render_mesh_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* meshComp = reg.get_component<mesh_component>(e);
        if (!meshComp) return;

        ImGui::Text("Mesh ID: %u", meshComp->mesh_id);
        ImGui::Text("Loaded File: %s", meshComp->model_path.c_str());

        // Dinamik Model Yükleme Tetikleyicisi
        if (ImGui::Button("Load New Model (.obj)")) {
            NFD::UniquePath outPath;
            nfdfilteritem_t filterItem[1] = { { "Wavefront OBJ", "obj" } };

            nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1, nullptr);

            if (result == NFD_OKAY) {
                std::string selectedPath = outPath.get();
                auto importResult = asset_pipeline::import_model(engineCtx, selectedPath);

                if (!importResult.mesh_ids.empty()) {
                    meshComp->mesh_id = importResult.mesh_ids.front();
                    meshComp->model_path = selectedPath;

                    if (!reg.has_component<bounds_component>(e))
                        reg.add_component<bounds_component>(e);

                    auto* bounds = reg.get_component<bounds_component>(e);
                    bounds->localCenter = importResult.boundsCenter;
                    bounds->localRadius = importResult.boundsRadius;
                }
            }
            else if (result == NFD_ERROR) {
                std::cout << "[NFD Error] " << NFD_GetError() << std::endl;
            }
        }
    }
}

inline void add_mesh_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<mesh_component>(e)) {
        reg.add_component<mesh_component>(e);
    }
}

inline void register_mesh_ui_system() {
    ComponentUIInfo info;
    info.name = "Mesh Renderer";
    info.render_fn = render_mesh_ui;
    info.add_fn = add_mesh_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<mesh_component>(e);
        };

    component_ui_registry::register_component(typeid(mesh_component).hash_code(), info);
}