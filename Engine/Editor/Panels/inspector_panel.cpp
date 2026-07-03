#include "inspector_panel.h"
#include "../ComponentUI/component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include "asset_browser_panel.h" 
#include "../IAssetInspector.h"
#include "../AssetUI/material_inspector.h"
#include <imgui.h>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm> // std::transform için

inspector_panel::inspector_panel() {
    m_asset_inspectors["material"] = std::make_unique<aria::editor::MaterialInspector>();
}

inspector_panel::~inspector_panel() = default;

std::string inspector_panel::get_asset_type_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "❌ [Inspector] Dosya acilamadi: " << path << std::endl;
        return "";
    }

    char buffer[512] = { 0 };
    file.read(buffer, sizeof(buffer) - 1);
    std::string content(buffer);

    // 🚀 GÜVENCE: Önce "type" ara, bulamazsan "asset_type" ara! 
    size_t pos = content.find("\"type\"");
    if (pos == std::string::npos) {
        pos = content.find("\"asset_type\"");
    }

    if (pos != std::string::npos) {
        size_t colon = content.find(":", pos);
        if (colon != std::string::npos) {
            size_t first_quote = content.find("\"", colon);
            size_t second_quote = content.find("\"", first_quote + 1);

            if (first_quote != std::string::npos && second_quote != std::string::npos) {
                std::string detected_type = content.substr(first_quote + 1, second_quote - first_quote - 1);
                std::cout << "🔍 [Inspector] Dosyadan okunan tur: " << detected_type << std::endl;
                return detected_type;
            }
        }
    }

    std::cout << "⚠️ [Inspector] JSON icinde tip belirteci bulunamadı!" << std::endl;
    return "";
}

void inspector_panel::render(scene& scene, int selectedEntity, EngineContext& engineCtx, EditorContext& editorCtx) {
    ImGui::Begin("Inspector");

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        editorCtx.focusedPanel = FocusedPanel::Inspector;
    }

    // ========================================================================
    // 🔥 AKILLI MODÜLER ASSET DAĞITIMI
    // ========================================================================
    std::string asset_path = editorCtx.selectedAssetPath;

    if (!asset_path.empty()) {
        std::filesystem::path p(asset_path);
        std::string ext = p.extension().string();

        // Uzantıyı küçük harfe çevirerek Windows/Linux uyuşmazlığını çözüyoruz
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".json" && std::filesystem::exists(p)) {
            // Dosyanın alt türünü doğrula
            std::string asset_type = get_asset_type_from_file(asset_path);

            // Eğer bu dosya tipi sistemimize kayıtlı bir modül ise (örn: "material")
            auto it = m_asset_inspectors.find(asset_type);
            if (it != m_asset_inspectors.end()) {
                it->second->on_render_inspector(asset_path, editorCtx);
                ImGui::End();
                return; // Çizimi bitir, alt taraftaki ECS katmanına geçme!
            }
        }
    }

    // =========================================================
    // ESKİ DOĞAL ECS ENTITY INSPECTOR KODLARI (Aynen korundu)
    // =========================================================
    if (selectedEntity == -1) {
        ImGui::Text("No entity selected");
        ImGui::End();
        return;
    }

    auto& reg = scene.get_registry();
    entity e = (entity)selectedEntity;

    ImGui::Text("Entity %d", selectedEntity);
    ImGui::Separator();

    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("add_component_popup");
    }

    const auto& registry_map = component_ui_registry::get_registry();

    if (ImGui::BeginPopup("add_component_popup")) {
        for (const auto& [type_hash, ui_info] : registry_map) {
            if (ui_info.has_fn && !ui_info.has_fn(e, reg)) {
                if (ImGui::MenuItem(ui_info.name)) {
                    if (ui_info.add_fn) {
                        ui_info.add_fn(e, reg, engineCtx, editorCtx);
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    for (const auto& [type_hash, ui_info] : registry_map) {
        if (ui_info.has_fn && ui_info.has_fn(e, reg)) {
            ImGui::PushID((int)type_hash);

            float button_width = 55.0f;
            float button_height = 18.0f;
            float header_height = ImGui::GetFrameHeight();
            ImVec2 backup_cursor_pos = ImGui::GetCursorPos();
            float content_width = ImGui::GetContentRegionAvail().x;

            float align_y = (header_height - button_height) * 0.5f;
            ImGui::SetCursorPos(ImVec2(backup_cursor_pos.x + content_width - button_width - 4.0f, backup_cursor_pos.y + align_y));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

            bool remove_clicked = false;
            std::string comp_name = ui_info.name ? ui_info.name : "";
            if (comp_name != "Name" && comp_name != "Transform" && ui_info.remove_fn) {
                remove_clicked = ImGui::Button("Remove", ImVec2(button_width, button_height));
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);

            ImGui::SetCursorPos(backup_cursor_pos);
            bool open = ImGui::CollapsingHeader(ui_info.name, ImGuiTreeNodeFlags_DefaultOpen);

            if (remove_clicked && ui_info.remove_fn) {
                ui_info.remove_fn(e, reg);
                ImGui::PopID();
                continue;
            }

            if (open && ui_info.render_fn) {
                ImGui::Spacing();
                ui_info.render_fn(e, reg, engineCtx, editorCtx);
                ImGui::Spacing();
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    ImGui::End();
}