#include "inspector_panel.h"
#include "../ComponentUI/component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>
#include <string> // std::string kontrolü için eklendi

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
    // DİNAMİK BİLEŞEN ÇİZİMLERİ & MERKEZİ BAŞLIK / SİLME YÖNETİMİ
    // =========================================================
    for (const auto& [type_hash, ui_info] : registry_map) {
        if (ui_info.has_fn && ui_info.has_fn(e, reg)) {

            // ImGui ID çakışmalarını önlemek için her bileşene benzersiz ID alanı açıyoruz
            ImGui::PushID((int)type_hash);

            // 1. "Remove" Butonunun Konum ve Stil Hesaplamaları
            float button_width = 55.0f;
            float button_height = 18.0f;
            float header_height = ImGui::GetFrameHeight();
            ImVec2 backup_cursor_pos = ImGui::GetCursorPos();
            float content_width = ImGui::GetContentRegionAvail().x;

            // Butonu katman olarak en üstte çizmek ve tıklamayı yutmasını sağlamak için Header'dan önce çiziyoruz
            float align_y = (header_height - button_height) * 0.5f;
            ImGui::SetCursorPos(ImVec2(backup_cursor_pos.x + content_width - button_width - 4.0f, backup_cursor_pos.y + align_y));

            // Buton için tatlı bir koyu kırmızı editör stili ayarlayalım
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f); // Hafif yuvarlatılmış köşeler

            bool remove_clicked = false;

            // Kritik Güvenlik Filtresi: "Name" veya "Transform" gibi bir entity'nin olmazsa olmaz bileşenlerinin silinmesini engelliyoruz
            std::string comp_name = ui_info.name ? ui_info.name : "";
            if (comp_name != "Name" && comp_name != "Transform" && ui_info.remove_fn) {
                remove_clicked = ImGui::Button("Remove", ImVec2(button_width, button_height));
            }

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);

            // 2. İmleci Geri Sar ve Jenerik Başlığı Çiz
            ImGui::SetCursorPos(backup_cursor_pos);
            bool open = ImGui::CollapsingHeader(ui_info.name, ImGuiTreeNodeFlags_DefaultOpen);

            // 3. Merkezi Silme Tetikleyicisi (Butona basıldıysa)
            if (remove_clicked && ui_info.remove_fn) {
                ui_info.remove_fn(e, reg); // `std::function` sarmalı çağrılır, RAM temizliği ve ECS silme işlemi yapılır
                ImGui::PopID();
                continue; // Bileşen silindiği için alt tarafı çizmeden döngüdeki bir sonraki bileşene atla
            }

            // 4. Bileşenin Kendi Özel İçeriğini Çiz (Eğer başlık açıksa)
            if (open && ui_info.render_fn) {
                ImGui::Spacing();
                ui_info.render_fn(e, reg, engineCtx, editorCtx); // Diğer dosyalardaki Combo Box, DragFloat vb. özellikleri çizer
                ImGui::Spacing();
            }

            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    ImGui::End();
}