#include "top_bar_panel.h"
#include <imgui.h>
#include "../../Scene/scene.h"                  // scene sınıfı için
#include "../../Scene/scene_serializer.h"       // serializer sınıfı için
#include "../../Externals/nfd/nfd.h"            // NFD dosya diyaloğu için (Yolunu kontrol et)
#include <iostream>
#include <imgui_internal.h>

void top_bar_panel::render(EngineMode& mode, scene& currentScene, EngineContext& ctx)
{
    ImGui::Begin("TopBar");

    // 1. Oynat / Durdur Butonu
    if (ImGui::Button(mode == EngineMode::EDITOR ? "Play" : "Stop"))
        mode = (mode == EngineMode::EDITOR) ? EngineMode::PLAY : EngineMode::EDITOR;

    ImGui::SameLine();
    ImGui::Text("Mode: %s", mode == EngineMode::EDITOR ? "EDITOR" : "PLAY");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // 2. 💾 SAHNEYİ KAYDET (SAVE SCENE)
    if (ImGui::Button("Save Scene"))
    {
        nfdchar_t* outPath = nullptr;
        nfdfilteritem_t filterItem[1] = { { "Engine Scene", "json" } };

        // NFD_SaveDialog_With_Filter_26: Projendeki güncel NFD sürümüne göre 
        // eğer fonksiyon adı sadece NFD_SaveDialog ise parametreleri (outPath, filterItem, 1, nullptr, "scene.json") yapabilirsin.
        nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "scene.json");

        if (result == NFD_OKAY)
        {
            std::string filepath(outPath);
            scene_serializer::serialize(filepath, currentScene, ctx);
            NFD_FreePath(outPath); // Bellek sızıntısını önlemek için yolu temizle
        }
    }

    ImGui::SameLine();

    // 3. 📂 SAHNEYİ YÜKLE (LOAD SCENE)
    if (ImGui::Button("Load Scene"))
    {
        nfdchar_t* outPath = nullptr;
        nfdfilteritem_t filterItem[1] = { { "Engine Scene", "json" } };

        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY)
        {
            std::string filepath(outPath);
            scene_serializer::deserialize(filepath, currentScene, ctx);
            NFD_FreePath(outPath);
        }
    }

    ImGui::End();
}