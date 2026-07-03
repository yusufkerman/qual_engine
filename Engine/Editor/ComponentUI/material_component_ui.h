/**
 * @file material_component_ui.h
 * @brief Materyal bileşeni (material_component) için editör kullanıcı arayüzü yönetim sistemi.
 */
#pragma once
#include "component_ui_registry.h"
#include "UIHelper.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Renderer/material.h"
#include "../../Externals/nfd/nfd.hpp"
#include "../../Externals/nfd/nfd.h"
#include <imgui.h>
#include <iostream>

inline void render_material_ui(entity e,
    registry& reg,
    EngineContext& engineCtx,
    EditorContext&)
{
    auto* matComp = reg.get_component<material_component>(e);
    if (!matComp) return;

    aria::system* ariaSys = engineCtx.aria_system;
    if (!ariaSys) return;

    aria::material_asset* current = nullptr;

    if (matComp->material_handle.is_valid())
        current = ariaSys->registry()->get_material(matComp->material_handle);

    // İlk başta label'ı güvenle alıyoruz
    std::string label = current ? current->name : "";

    aria::UI::DrawAssetDropSlot(
        "Material",
        label,
        aria::editor::asset_payload_type::material,
        [&](const aria::editor::asset_drag_payload& payload)
        {
            std::cout << "[MAT UI DEBUG] Drop callback TETIKLENDI, path=" << payload.path << "\n";

            auto handle = asset_pipeline::import_material(engineCtx, payload.path);

            if (handle.is_valid()) {
                matComp->material_handle = handle;
                std::cout << "[MAT UI DEBUG] material_handle ATANDI!\n";

                // 🛡️ EMNİYET KİLİDİ 1: Eski materyal silinmiş olabileceği için 
                // Lambda içinde 'current' pointer'ını yeni handle ile hemen güncelliyoruz!
                current = ariaSys->registry()->get_material(handle);
            }
            else {
                std::cout << "[MAT UI DEBUG] Handle INVALID, atama YAPILMADI!\n";
            }
        }
    );

    // 🛡️ EMNİYET KİLİDİ 2: Lambda çalıştıktan sonra eğer hafıza silindiyse 
    // veya handle değiştiyse pointer'ı bir kez daha teyit ediyoruz.
    if (matComp->material_handle.is_valid()) {
        current = ariaSys->registry()->get_material(matComp->material_handle);
    }
    else {
        current = nullptr;
    }

    // Artık 'current' pointer'ının güncel ve güvenli olduğundan %100 eminiz.
    if (current)
        ImGui::Text("Material: %s", current->name.c_str());
    else
        ImGui::Text("Material: None");
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

    info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<material_component>(e);
        };

    component_ui_registry::register_component(typeid(material_component).hash_code(), info);
}