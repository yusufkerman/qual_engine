#pragma once

/**
 * @file material_component_ui.h
 * @brief Materyal bileşeni (material_component) için editör kullanıcı arayüzü ve sürükle-bırak slot sistemi.
 */

#include "../../Services/component_operation_registry.h"
#include "../Vesta/Services/component_ui_registry.h"
#include "../../../UIHelper.h"
#include "../../ECS/registry.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Renderer/material.h"
#include <imgui.h>
#include <iostream>

 /**
  * @brief Seçili varlığın materyal slotunu ve sürükle-bırak (Drag & Drop) etkileşimini çizer.
  * @param e Materyal atanacak varlık.
  * @param reg İlgili ECS kayıt defteri referansı.
  * @param ariaSys Varlık çözümleme için Aria çekirdek sistemi referansı.
  */
inline void render_material_ui(entity e, registry& reg, aria::system& ariaSys)
{
    auto* matComp = reg.get_component<material_component>(e);
    if (!matComp) return;

    aria::material_asset* current = nullptr;

    if (matComp->material_handle.is_valid()) {
        current = ariaSys.registry()->get_material(matComp->material_handle);
    }

    std::string label = current ? current->name : "";

    vesta::DrawAssetDropSlot(
        "Material",
        label,
        vesta::asset_payload_type::material,
        [&](const vesta::asset_drag_payload& payload)
        {
            auto handle = asset_pipeline::reload_material(&ariaSys, payload.path);

            if (handle.is_valid()) {
                matComp->material_handle = handle;
                current = ariaSys.registry()->get_material(handle);
            }
            else {
                std::cerr << "[Vesta MaterialUI] Materyal ice aktarilamadi: " << payload.path << "\n";
            }
        }
    );

    // Drop işlemi sonrası handle geçerliliğini ve pointer bütünlüğünü doğrula
    if (matComp->material_handle.is_valid()) {
        current = ariaSys.registry()->get_material(matComp->material_handle);
    }
    else {
        current = nullptr;
    }

    if (current) {
        ImGui::Text("Material: %s", current->name.c_str());
    }
    else {
        ImGui::Text("Material: None");
    }
}

/**
 * @brief Bir varlığa varsayılan material_component ekler.
 */
inline void add_material_component(entity e, registry& reg) {
    if (!reg.has_component<material_component>(e)) {
        reg.add_component<material_component>(e);
    }
}

/**
 * @brief Materyal bileşeni arayüz ve operasyon kayıtlarını gerçekleştirir.
 * @param ariaSys Çizim fonksiyonuna closure ile bağlanacak olan Aria sistemi referansı.
 */
inline void register_material_ui_system(aria::system& ariaSys) {
    size_t comp_hash = typeid(material_component).hash_code();

    // 1. ECS Operasyon Kaydı
    vesta::ComponentOperationInfo logic;
    logic.add_fn = add_material_component;

    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<material_component>(e);
        };

    logic.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<material_component>(e);
        };

    vesta::component_operation_registry::register_component(comp_hash, logic);

    // 2. UI Çizim Kaydı (Aria sistemi closure içerisine yakalanır)
    vesta::ComponentUIInfo ui;
    ui.display_name = "Material";
    ui.render_fn = [&ariaSys](entity e, registry& reg) {
        render_material_ui(e, reg, ariaSys);
        };

    vesta::component_ui_registry::register_component(comp_hash, ui);
}