#include "inspector_panel.h"
#include "../ComponentUI/component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"

#include <imgui.h>

void inspector_panel::render(scene& scene, int selectedEntity, EngineContext& engineCtx, EditorContext& editorCtx) {
    ImGui::Begin("Inspector");

    // Pencere odağını güncelleme mantığı (Aynen korundu)
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        editorCtx.focusedPanel = FocusedPanel::Inspector;
    }

    // Seçili entity kontrolü (int tabanlı ve -1 kontrolü aynen korundu)
    if (selectedEntity == -1) {
        ImGui::Text("No entity selected");
        ImGui::End();
        return;
    }

    auto& reg = scene.get_registry();
    entity e = (entity)selectedEntity;

    ImGui::Text("Entity %d", selectedEntity);
    ImGui::Separator();

    // =========================================================
    // DİNAMİK "ADD COMPONENT" MENÜSÜ
    // =========================================================
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("add_component_popup");
    }

    const auto& registry_map = component_ui_registry::get_registry();

    if (ImGui::BeginPopup("add_component_popup")) {
        for (const auto& [type_hash, ui_info] : registry_map) {
            // Eğer bileşene sahip değilse menüde ekleme seçeneği olarak göster
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

    // =========================================================
    // DİNAMİK BİLEŞEN ÇİZİMLERİ (RENDER)
    // =========================================================
    for (const auto& [type_hash, ui_info] : registry_map) {
        if (ui_info.has_fn && ui_info.has_fn(e, reg)) {
            if (ui_info.render_fn) {
                ui_info.render_fn(e, reg, engineCtx, editorCtx);
                ImGui::Spacing();
            }
        }
    }

    ImGui::End();
}