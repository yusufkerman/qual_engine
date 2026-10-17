/**
 * @file top_bar_panel.cpp
 * @brief Üst kontrol çubuğu paneli ve Window açılır menüsü çizim uygulaması.
 */

#include "top_bar_panel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "../../Externals/nfd/nfd.h"

namespace vesta {

    top_bar_panel::top_bar_panel(top_bar_view_model& vm, const std::vector<IEditorPanel*>* panels)
        : m_vm(vm)
        , m_panels(panels)
    {
    }

    IEditorPanel* top_bar_panel::find_panel(const std::string& title) const
    {
        if (!m_panels) return nullptr;
        for (auto* p : *m_panels)
        {
            if (p && p->get_title() == title)
            {
                return p;
            }
        }
        return nullptr;
    }

    void top_bar_panel::render()
    {
        if (!m_is_open)
            return;

        ImGui::Begin(m_title.c_str(), &m_is_open);

        const bool isPlaying = m_vm.is_playing();
        const bool isPaused  = m_vm.is_paused();

        // 1. Oynat / Devam Et (Play / Resume) Butonu
        const bool playActive = isPlaying && !isPaused;
        if (playActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.62f, 0.34f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.72f, 0.40f, 1.0f));
        }
        if (ImGui::Button(isPaused ? "Resume" : "Play"))
        {
            m_vm.play();
            ImGui::SetWindowFocus("Game");
        }
        if (playActive)
        {
            ImGui::PopStyleColor(2);
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(isPaused ? "Resume Simulation" : "Start Play Mode");
        }

        ImGui::SameLine(0.0f, 4.0f);

        // 2. Duraklat (Pause) Butonu
        ImGui::BeginDisabled(!isPlaying);
        if (isPaused)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.78f, 0.54f, 0.14f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.64f, 0.18f, 1.0f));
        }
        if (ImGui::Button("Pause"))
        {
            m_vm.toggle_pause();
        }
        if (isPaused)
        {
            ImGui::PopStyleColor(2);
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Pause / Resume Simulation (Freeze physics & scripts while inspecting live world)");
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0.0f, 4.0f);

        // 3. Durdur (Stop) Butonu
        ImGui::BeginDisabled(!isPlaying);
        if (isPlaying)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.22f, 0.22f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.28f, 0.28f, 1.0f));
        }
        if (ImGui::Button("Stop"))
        {
            m_vm.stop();
        }
        if (isPlaying)
        {
            ImGui::PopStyleColor(2);
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Stop Play Mode and restore pre-play scene snapshot");
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (!isPlaying)
        {
            ImGui::Text("Mode: EDITOR");
        }
        else if (isPaused)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.24f, 1.0f), "Mode: PAUSED");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.52f, 1.0f), "Mode: PLAYING");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Oyun modundayken sahne dosyasının üzerine yanlışlıkla runtime durumu yazılmasını önle
        ImGui::BeginDisabled(isPlaying);

        // Sahne Kaydetme (Save Scene Dialog)
        if (ImGui::Button("Save Scene"))
        {
            nfdchar_t* outPath = nullptr;
            nfdfilteritem_t filterItem[1] = { { "Engine Scene", "json" } };

            if (NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "scene.json") == NFD_OKAY)
            {
                if (outPath != nullptr)
                {
                    m_vm.execute_save(std::string(outPath));
                    NFD_FreePath(outPath);
                }
            }
        }
        if (isPlaying && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Stop Play Mode before saving the scene");
        }

        ImGui::SameLine();

        // Sahne Yükleme (Load Scene Dialog)
        if (ImGui::Button("Load Scene"))
        {
            nfdchar_t* outPath = nullptr;
            nfdfilteritem_t filterItem[1] = { { "Engine Scene", "json" } };

            if (NFD_OpenDialog(&outPath, filterItem, 1, nullptr) == NFD_OKAY)
            {
                if (outPath != nullptr)
                {
                    m_vm.execute_load(std::string(outPath));
                    NFD_FreePath(outPath);
                }
            }
        }
        if (isPlaying && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Stop Play Mode before loading a scene");
        }

        ImGui::EndDisabled();

        ImGui::End();
    }

} // namespace vesta