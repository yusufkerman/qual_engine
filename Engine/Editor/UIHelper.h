#pragma once
#include <imgui.h>
#include <string>
#include <functional>
#include "Vesta/Services/asset_entry.h"

namespace vesta {

    inline void DrawAssetDropSlot(
        const char* label,
        const std::string& current_name,
        vesta::asset_payload_type expected_type,
        std::function<void(const vesta::asset_drag_payload&)> on_drop)
    {
        ImGui::Text("%s", label);

        ImVec4 col = current_name.empty()
            ? ImVec4(0.2f, 0.2f, 0.2f, 1.0f)
            : ImVec4(0.15f, 0.35f, 0.15f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, col);
        ImGui::Button(current_name.empty() ? "None" : current_name.c_str(),
            ImVec2(ImGui::GetContentRegionAvail().x, 26));
        ImGui::PopStyleColor();

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* p =
                ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD"))
            {
                auto* payload = (const vesta::asset_drag_payload*)p->Data;

                if (payload->type == expected_type)
                    on_drop(*payload);
            }
            ImGui::EndDragDropTarget();
        }
    }

}