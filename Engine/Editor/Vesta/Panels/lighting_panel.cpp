/**
 * @file lighting_panel.cpp
 * @brief Sahne ışıklandırma ve üstel sis (Exponential Fog) ayarları arayüz uygulaması.
 */

#include "lighting_panel.h"
#include "../../../Engine/engine_context.h"
#include <imgui.h>

namespace vesta {

    lighting_panel::lighting_panel(EngineContext& engineCtx)
        : m_engine_ctx(engineCtx)
    {
    }

    void lighting_panel::render()
    {
        if (!m_is_open)
            return;

        ImGui::Begin(m_title.c_str(), &m_is_open);

        ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Environment & Lighting Settings");
        ImGui::Separator();

        // ====================================================================
        // 1. ÜSTEL SİS (EXPONENTIAL FOG) AYARLARI
        // ====================================================================
        if (ImGui::CollapsingHeader("Fog (Exponential)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& fog = m_engine_ctx.fog;

            ImGui::Checkbox("Enable Fog", &fog.enabled);
            ImGui::SameLine();
            ImGui::Checkbox("Show in Scene View", &fog.show_in_scene);

            ImGui::Spacing();
            ImGui::ColorEdit3("Fog Color", &fog.color.x, ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("Density", &fog.density, 0.0001f, 0.0400f, "%.4f");

            ImGui::Spacing();
            ImGui::TextDisabled("Quick Fog Presets:");
            if (ImGui::SmallButton("Clear / Subtle"))
            {
                fog.enabled = true;
                fog.density = 0.003f;
                fog.color = glm::vec3(0.70f, 0.80f, 0.90f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Morning Mist"))
            {
                fog.enabled = true;
                fog.density = 0.010f;
                fog.color = glm::vec3(0.75f, 0.82f, 0.92f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Dense Fog"))
            {
                fog.enabled = true;
                fog.density = 0.025f;
                fog.color = glm::vec3(0.60f, 0.65f, 0.70f);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Sunset / Dusk"))
            {
                fog.enabled = true;
                fog.density = 0.009f;
                fog.color = glm::vec3(0.85f, 0.45f, 0.30f);
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 2. SAHNE IŞIKLARI ÖZETİ
        // ====================================================================
        if (ImGui::CollapsingHeader("Scene Lights Summary", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto& lightBatch = m_engine_ctx.session.editor_scene().get_light_batch().lights;
            ImGui::Text("Active Lights in Scene: %d", static_cast<int>(lightBatch.size()));

            int dirCount = 0, pointCount = 0, spotCount = 0;
            for (const auto& l : lightBatch)
            {
                if (l.type == 0) dirCount++;
                else if (l.type == 1) pointCount++;
                else if (l.type == 2) spotCount++;
            }

            ImGui::BulletText("Directional Lights: %d", dirCount);
            ImGui::BulletText("Point Lights: %d", pointCount);
            ImGui::BulletText("Spot Lights: %d", spotCount);

            if (dirCount == 0)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Warning: No Directional Light in scene for shadows.");
            }
        }

        ImGui::End();
    }

} // namespace vesta

