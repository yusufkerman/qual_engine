#pragma once

/**
 * @file material_component_ui.h
 * @brief Materyal bileşeni (material_component) için editör kullanıcı arayüzü ve sürükle-bırak slot sistemi.
 * @note Kullanıcının tercihine göre PBR parametreleri bileşen üzerinde değil, Asset Browser üzerinden 
 * açılan Material Inspector panelinde düzenlenir.
 */

#include "../../Services/component_operation_registry.h"
#include "../Vesta/Services/component_ui_registry.h"
#include "../../../UIHelper.h"
#include "../../ECS/registry.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Renderer/material.h"
#include "../../../Core/path_utils.h"
#include "../../../Externals/nlohmann/json.hpp"
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <iostream>

/**
 * @brief Bellekteki materyal varlığının PBR parametrelerini diskteki .json dosyasına senkronize eder (Dışa aktarım ve kaydetme için).
 */
inline void sync_material_asset_to_disk(const aria::material_asset& mat, aria::system& ariaSys)
{
    if (mat.guid.is_nil()) return;
    const std::string path = ariaSys.resolve_asset_path(mat.guid);
    const auto fs_path = qual::path_from_utf8(path);
    if (path.empty() || !std::filesystem::exists(fs_path)) return;

    try {
        nlohmann::json j;
        {
            std::ifstream inFile(fs_path);
            if (inFile.is_open()) {
                inFile >> j;
            }
        }
        j["type"] = "material";
        const std::string live_stem = qual::path_to_utf8(fs_path.stem());
        if (!live_stem.empty()) {
            j["name"] = live_stem;
        }
        else if (!mat.name.empty()) {
            j["name"] = mat.name;
        }
        j["base_color"] = { mat.base_color.r, mat.base_color.g, mat.base_color.b, 1.0f };
        j["metallic"] = mat.metallic;
        j["roughness"] = mat.roughness;
        j["ao"] = mat.ao;
        j["emissive_color"] = { mat.emissive_color.r, mat.emissive_color.g, mat.emissive_color.b };
        j["emissive_intensity"] = mat.emissive_intensity;
        j["normal_strength"] = mat.normal_strength;
        j["texture_scale"] = { mat.texture_scale.x, mat.texture_scale.y };
        j["render_mode"] = static_cast<int>(mat.mode);
        j["alpha_threshold"] = mat.alpha_threshold;

        std::ofstream outFile(fs_path);
        if (outFile.is_open()) {
            outFile << j.dump(2);
        }
    }
    catch (...) {}
}

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

    // Materyal yeniden adlandırıldıysa (rename), GUID üzerinden güncel dosya adını senkronize et
    if (current && !current->guid.is_nil()) {
        const std::string resolved_path = ariaSys.resolve_asset_path(current->guid);
        if (!resolved_path.empty()) {
            const std::string live_stem = qual::path_to_utf8(qual::path_from_utf8(resolved_path).stem());
            if (!live_stem.empty() && current->name != live_stem) {
                current->name = live_stem;
            }
        }
    }

    std::string label = current ? current->name : "";

    vesta::DrawAssetDropSlot(
        "Material",
        label,
        vesta::asset_payload_type::material,
        [&](const vesta::asset_drag_payload& payload)
        {
            auto handle = asset_pipeline::reload_material(&ariaSys, payload.path);
            if (!handle.is_valid() && !payload.guid.is_nil()) {
                handle = ariaSys.get_or_load_asset_by_guid<aria::material_asset>(payload.guid);
            }

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
        if (current && !current->guid.is_nil()) {
            const std::string resolved_path = ariaSys.resolve_asset_path(current->guid);
            if (!resolved_path.empty()) {
                const std::string live_stem = qual::path_to_utf8(qual::path_from_utf8(resolved_path).stem());
                if (!live_stem.empty() && current->name != live_stem) {
                    current->name = live_stem;
                }
            }
        }
    }
    else {
        current = nullptr;
    }

    if (current) {
        ImGui::TextDisabled("Assigned Material: %s", current->name.c_str());
        ImGui::TextDisabled("(Double-click material in Asset Browser to edit properties)");
    }
    else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No Material Assigned");
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
    ui.category = "Rendering";
    ui.render_fn = [&ariaSys](entity e, registry& reg) {
        render_material_ui(e, reg, ariaSys);
        };

    vesta::component_ui_registry::register_component(comp_hash, ui);
}
