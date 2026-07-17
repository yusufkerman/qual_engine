#include "top_bar_panel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "../../Externals/nfd/nfd.h"


namespace vesta {


    void top_bar_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);


        bool isEditor =
            (m_vm.get_mode() == EngineMode::EDITOR);


        if (ImGui::Button(isEditor ? "Play" : "Stop"))
        {
            m_vm.toggle_mode();
        }


        ImGui::SameLine();

        ImGui::Text(
            "Mode: %s",
            isEditor ? "EDITOR" : "PLAY"
        );


        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();


        if (ImGui::Button("Save Scene"))
        {
            nfdchar_t* outPath = nullptr;

            nfdfilteritem_t filterItem[1] =
            {
                { "Engine Scene", "json" }
            };


            if (NFD_SaveDialog(
                &outPath,
                filterItem,
                1,
                nullptr,
                "scene.json") == NFD_OKAY)
            {
                m_vm.execute_save(
                    std::string(outPath)
                );

                NFD_FreePath(outPath);
            }
        }


        ImGui::SameLine();


        if (ImGui::Button("Load Scene"))
        {
            nfdchar_t* outPath = nullptr;

            nfdfilteritem_t filterItem[1] =
            {
                { "Engine Scene", "json" }
            };


            if (NFD_OpenDialog(
                &outPath,
                filterItem,
                1,
                nullptr) == NFD_OKAY)
            {
                m_vm.execute_load(
                    std::string(outPath)
                );

                NFD_FreePath(outPath);
            }
        }


        ImGui::End();
    }


}