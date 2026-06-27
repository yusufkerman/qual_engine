#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Renderer/material.h"
#include "../../Externals/nfd/nfd.hpp"
#include "../../Externals/nfd/nfd.h"
#include <imgui.h>

inline void render_material_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* matComp = reg.get_component<material_component>(e);
        if (!matComp) return;

        material* mat = engineCtx.assetManager.try_get<material>(matComp->material_id);

        if (mat) {
            ImGui::Text("Material ID: %u", matComp->material_id);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Shared Material - Edits affect all clones)");

            if (ImGui::Button("Make Unique (Clone Material)")) {
                material clonedMat = *mat;
                asset_id newMatID = engineCtx.assetManager.add<material>(clonedMat);
                matComp->material_id = newMatID;
                mat = engineCtx.assetManager.try_get<material>(newMatID);
            }

            ImGui::Separator();
            ImGui::ColorEdit3("Base Color", &mat->base_color.x);
            ImGui::SliderFloat("Normal Strength", &mat->normal_strength, 0.0f, 3.0f);
            ImGui::DragFloat2("Texture Tiling", &mat->texture_scale.x, 0.05f, 0.01f, 100.0f, "%.2f");

            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                mat->texture_scale = glm::vec2(1.0f);
            }

            ImGui::Separator();
            ImGui::Text("Diffuse Texture ID: %u", mat->diffuse_texture);
            if (ImGui::Button("Load Diffuse Texture##NFD")) {
                NFD::UniquePath outPath;
                nfdfilteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga,bmp" } };
                if (NFD::OpenDialog(outPath, filterItem, 1, nullptr) == NFD_OKAY) {
                    asset_id texID = asset_pipeline::import_texture(engineCtx, outPath.get());
                    if (texID != 0) mat->diffuse_texture = texID;
                }
            }

            ImGui::Separator();
            ImGui::Text("Normal Map ID: %u", mat->normal_texture);
            if (ImGui::Button("Load Normal Map##NFD")) {
                NFD::UniquePath outPath;
                nfdfilteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga,bmp" } };
                if (NFD::OpenDialog(outPath, filterItem, 1, nullptr) == NFD_OKAY) {
                    asset_id texID = asset_pipeline::import_texture(engineCtx, outPath.get());
                    if (texID != 0) mat->normal_texture = texID;
                }
            }
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: No material assigned!");
            if (ImGui::Button("Create & Assign New Material")) {
                material defaultMat;
                defaultMat.shader_id = 0;
                defaultMat.base_color = glm::vec3(1.0f);
                defaultMat.normal_strength = 1.0f;
                defaultMat.diffuse_texture = 0;
                defaultMat.normal_texture = 0;
                defaultMat.texture_scale = glm::vec2(1.0f);
                matComp->material_id = engineCtx.assetManager.add<material>(defaultMat);
            }
        }
    }
}

inline void add_material_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<material_component>(e)) {
        reg.add_component<material_component>(e);
    }
}

inline void register_material_ui_system() {
    ComponentUIInfo info;
    info.name = "Material";
    info.render_fn = render_material_ui;
    info.add_fn = add_material_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<material_component>(e);
        };

    component_ui_registry::register_component(typeid(material_component).hash_code(), info);
}