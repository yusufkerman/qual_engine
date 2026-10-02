#pragma once

/**
 * @file UIHelper.h
 * @brief Editör panellerinde ortak kullanılan ImGui arayüz yardımcı fonksiyonlarını barındırır.
 */

#include <imgui.h>
#include <string>
#include <functional>
#include "Vesta/Services/asset_entry.h"

namespace vesta {

    /**
     * @brief Varlık tarayıcısından sürüklenebilen öğeler için hedef bırakma yuvası (Drop Slot) çizer.
     *
     * @param label Yuvanın üzerinde gösterilecek metin/etiket.
     * @param current_name Yuva içinde anlık atanmış olan varlığın adı (boşsa "None" gösterilir).
     * @param expected_type Yuvanın kabul edeceği varlık türü (Model, Texture, Material vb.).
     * @param on_drop Geçerli bir varlık bırakıldığında çalıştırılacak geri çağrım (callback) fonksiyonu.
     */
    inline void DrawAssetDropSlot(
        const char* label,
        const std::string& current_name,
        vesta::asset_payload_type expected_type,
        std::function<void(const vesta::asset_drag_payload&)> on_drop)
    {
        ImGui::Text("%s", label);

        // Dolu/boş durumuna göre yuva buton rengini ayarla
        ImVec4 col = current_name.empty()
            ? ImVec4(0.2f, 0.2f, 0.2f, 1.0f)
            : ImVec4(0.15f, 0.35f, 0.15f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, col);
        ImGui::Button(
            current_name.empty() ? "None" : current_name.c_str(),
            ImVec2(ImGui::GetContentRegionAvail().x, 26.0f)
        );
        ImGui::PopStyleColor();

        // Sürükle-Bırak hedef alanını denetle
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD"))
            {
                const auto* payload = static_cast<const vesta::asset_drag_payload*>(p->Data);

                if (payload && payload->type == expected_type && on_drop)
                {
                    on_drop(*payload);
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

} // namespace vesta