#include "top_bar_panel.h"
#include <imgui.h>


void top_bar_panel::render(EngineMode& mode)
{
    ImGui::Begin("TopBar");

    if (ImGui::Button(mode == EngineMode::EDITOR ? "Play" : "Stop"))
        mode = (mode == EngineMode::EDITOR)
        ? EngineMode::PLAY
        : EngineMode::EDITOR;

    ImGui::SameLine();
    ImGui::Text("Mode: %s",
        mode == EngineMode::EDITOR ? "EDITOR" : "PLAY");

    ImGui::End();
}