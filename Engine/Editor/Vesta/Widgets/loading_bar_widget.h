#pragma once

/**
 * @file loading_bar_widget.h
 * @brief Üst kontrol çubuğu (Top Bar) ve ana menü çubuğunda gösterilen, arka plan asenkron işlemlerini
 * (betik derleme, sahne/proje I/O) kullanıcıya anlık bildiren modern ve şık yükleme çubuğu bileşeni.
 */

#include <imgui.h>
#include <imgui_internal.h>
#include "../../../Async/async_task_manager.h"
#include <cmath>
#include <string>
#include <algorithm>

namespace vesta {

    /**
     * @brief Asenkron görev yöneticisinin durumuna göre modern, animasyonlu yükleme çubuğunu çizer.
     * @param width Çubuğun genişliği (piksel).
     * @param height Çubuğun yüksekliği (piksel).
     */
    inline void render_loading_bar_widget(float width = 280.0f, float height = 22.0f)
    {
        auto& taskMgr = core::async_task_manager::instance();
        if (!taskMgr.is_busy()) {
            return;
        }

        const float progress = taskMgr.get_progress();
        const std::string taskName = taskMgr.get_task_name();
        const std::string statusText = taskMgr.get_status_text();
        const std::string displayText = statusText.empty() ? taskName : statusText;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 screenPos = ImGui::GetCursorScreenPos();
        const ImVec2 barSize(width, height);
        const ImVec2 maxPos(screenPos.x + barSize.x, screenPos.y + barSize.y);

        const float rounding = barSize.y * 0.5f; // Şık ve modern hap (pill) geometrisi

        // 1. Arka plan katmanı (Koyu lacivert-füme, yarı saydam)
        drawList->AddRectFilled(screenPos, maxPos, IM_COL32(18, 22, 30, 240), rounding);
        drawList->AddRect(screenPos, maxPos, IM_COL32(52, 78, 115, 160), rounding, 0, 1.2f);

        // 2. İlerleme dolgusu veya animasyonlu dalga (Marquee)
        if (progress >= 0.0f && progress <= 1.0f)
        {
            // Belirli (Determinate) İlerleme: 0.0 - 1.0 arası dolgu
            const float clampedProgress = std::clamp(progress, 0.03f, 1.0f);
            const float fillWidth = barSize.x * clampedProgress;
            const ImVec2 fillMax(screenPos.x + fillWidth, maxPos.y);

            // Zengin elektrik mavisi gradient dolgu
            drawList->AddRectFilled(screenPos, fillMax, IM_COL32(32, 132, 245, 230), rounding);
            drawList->AddRect(screenPos, fillMax, IM_COL32(75, 175, 255, 180), rounding, 0, 1.0f);
        }
        else
        {
            // Belirsiz (Indeterminate) İlerleme: Yumuşak akıcı parlama dalgası
            const float time = static_cast<float>(ImGui::GetTime());
            const float marqueePillWidth = barSize.x * 0.35f;
            const float cycle = (std::sin(time * 3.5f) * 0.5f + 0.5f);
            const float startX = screenPos.x + (barSize.x - marqueePillWidth) * cycle;

            const ImVec2 marqueeMin(startX, screenPos.y);
            const ImVec2 marqueeMax(startX + marqueePillWidth, maxPos.y);

            drawList->AddRectFilled(marqueeMin, marqueeMax, IM_COL32(38, 148, 255, 200), rounding);
            drawList->AddRect(marqueeMin, marqueeMax, IM_COL32(90, 190, 255, 150), rounding, 0, 1.0f);
        }

        // 3. Yanıp sönen canlı neon aktivite noktası (Pulsing Dot)
        const float pulse = (std::sin(static_cast<float>(ImGui::GetTime()) * 6.0f) * 0.5f + 0.5f);
        const ImU32 dotColor = IM_COL32(
            static_cast<int>(60.0f + 195.0f * pulse),
            static_cast<int>(180.0f + 75.0f * pulse),
            255,
            255
        );
        drawList->AddCircleFilled(ImVec2(screenPos.x + 12.0f, screenPos.y + barSize.y * 0.5f), 3.5f, dotColor);

        // 4. Barın içerisindeki dinamik işlem metni (Metin gölgesi ile net okunabilirlik)
        std::string fullLabel = displayText;
        if (progress >= 0.0f && progress <= 1.0f) {
            char pctBuf[16];
            snprintf(pctBuf, sizeof(pctBuf), " (%d%%)", static_cast<int>(progress * 100.0f));
            fullLabel += pctBuf;
        }

        ImVec2 textSize = ImGui::CalcTextSize(fullLabel.c_str());
        const float maxTextWidth = barSize.x - 36.0f;
        if (textSize.x > maxTextWidth) {
            while (fullLabel.length() > 4 && ImGui::CalcTextSize((fullLabel + "...").c_str()).x > maxTextWidth) {
                fullLabel.pop_back();
            }
            fullLabel += "...";
            textSize = ImGui::CalcTextSize(fullLabel.c_str());
        }

        const ImVec2 textPos(
            screenPos.x + 22.0f + std::max(0.0f, (barSize.x - 26.0f - textSize.x) * 0.5f),
            screenPos.y + (barSize.y - textSize.y) * 0.5f
        );

        // Drop shadow & metin
        drawList->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 220), fullLabel.c_str());
        drawList->AddText(textPos, IM_COL32(245, 248, 255, 255), fullLabel.c_str());

        // ImGui imlecini ilerlet ve etkileşim alanı aç
        ImGui::Dummy(barSize);
        if (ImGui::IsItemHovered()) {
            std::string tooltipMsg = taskName;
            if (!statusText.empty() && statusText != taskName) {
                tooltipMsg += "\nStatus: " + statusText;
            }
            if (progress >= 0.0f) {
                tooltipMsg += "\nProgress: " + std::to_string(static_cast<int>(progress * 100.0f)) + "%";
            }
            ImGui::SetTooltip("%s", tooltipMsg.c_str());
        }
    }

} // namespace vesta

