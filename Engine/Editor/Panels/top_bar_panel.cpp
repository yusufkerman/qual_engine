#include "top_bar_panel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "../../Scene/scene.h"
#include "../../Externals/nfd/nfd.h"
#include "../../Sera/sera.h"

#include <iostream>

void top_bar_panel::render(EngineMode& mode, scene& currentScene, EngineContext& ctx)
{
    ImGui::Begin("TopBar");

    // 1. Oynat / Durdur
    if (ImGui::Button(mode == EngineMode::EDITOR ? "Play" : "Stop"))
    {
        mode = (mode == EngineMode::EDITOR)
            ? EngineMode::PLAY
            : EngineMode::EDITOR;
    }

    ImGui::SameLine();
    ImGui::Text("Mode: %s", mode == EngineMode::EDITOR ? "EDITOR" : "PLAY");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // 2. Sahneyi Kaydet
    if (ImGui::Button("Save Scene"))
    {
        nfdchar_t* outPath = nullptr;
        nfdfilteritem_t filterItem[1] =
        {
            { "Engine Scene", "json" }
        };

        nfdresult_t result =
            NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "scene.json");

        if (result == NFD_OKAY)
        {
            std::string filepath(outPath);

            if (ctx.sera)
            {
                sera::validation_report report =
                    ctx.sera->save_scene(filepath, currentScene);

                // report.messages burada loglanabilir.
            }

            NFD_FreePath(outPath);
        }
    }

    ImGui::SameLine();

    // 3. Sahneyi Yükle
    if (ImGui::Button("Load Scene"))
    {
        nfdchar_t* outPath = nullptr;
        nfdfilteritem_t filterItem[1] =
        {
            { "Engine Scene", "json" }
        };

        nfdresult_t result =
            NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY)
        {
            std::string filepath(outPath);

            if (ctx.sera)
            {
                sera::validation_report report =
                    ctx.sera->load_scene(filepath, currentScene);

                // report.messages burada loglanabilir.
            }

            NFD_FreePath(outPath);
        }
    }

    ImGui::End();
}