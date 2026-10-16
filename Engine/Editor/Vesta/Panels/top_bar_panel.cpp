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

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // ====================================================================
        // WINDOW MENÜSÜ (GENERAL VE RENDERING PENCERELERİ)
        // ====================================================================
        if (ImGui::Button("Window"))
        {
            ImGui::OpenPopup("TopBarWindowMenuPopup");
        }

        if (ImGui::BeginPopup("TopBarWindowMenuPopup"))
        {
            if (ImGui::BeginMenu("General"))
            {
                auto drawWindowItem = [this](const char* title) {
                    IEditorPanel* p = find_panel(title);
                    if (!p) return;
                    const bool isOpen = p->is_open();
                    // Zaten açık ise devre dışı (disabled) bırakılır, açılamaz
                    if (ImGui::MenuItem(title, nullptr, isOpen, !isOpen))
                    {
                        p->set_open(true);
                    }
                };

                drawWindowItem("Scene");
                drawWindowItem("Game");
                drawWindowItem("Hierarchy");
                drawWindowItem("Inspector");
                drawWindowItem("Asset Browser");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Rendering"))
            {
                IEditorPanel* p = find_panel("Lighting");
                if (p)
                {
                    const bool isOpen = p->is_open();
                    if (ImGui::MenuItem("Lighting", nullptr, isOpen, !isOpen))
                    {
                        p->set_open(true);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }

} // namespace vesta