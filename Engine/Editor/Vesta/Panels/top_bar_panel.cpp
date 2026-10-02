/**
 * @file top_bar_panel.cpp
 * @brief Üst kontrol çubuğu panelinin ImGui ve dosya seçici (NFD) çizim uygulaması.
 */

#include "top_bar_panel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "../../Externals/nfd/nfd.h"

namespace vesta {

    top_bar_panel::top_bar_panel(top_bar_view_model& vm)
        : m_vm(vm)
    {
    }

    void top_bar_panel::render()
    {
        if (!m_is_open)
            return;

        ImGui::Begin(m_title.c_str(), &m_is_open);

        bool isEditor = (m_vm.get_mode() == EngineMode::EDITOR);

        // Oynat / Durdur (Play / Stop) Durum Butonu
        if (ImGui::Button(isEditor ? "Play" : "Stop"))
        {
            m_vm.toggle_mode();
        }

        ImGui::SameLine();
        ImGui::Text("Mode: %s", isEditor ? "EDITOR" : "PLAY");

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

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

        ImGui::End();
    }

} // namespace vesta