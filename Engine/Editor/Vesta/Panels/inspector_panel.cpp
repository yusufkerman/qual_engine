#include "inspector_panel.h"
#include <imgui.h>

namespace vesta
{
    inspector_panel::inspector_panel(std::unique_ptr<inspector_view_model> vm)
        : m_vm(std::move(vm))
    {
    }

    void inspector_panel::render()
    {
        if (!m_is_open)
            return;

        m_vm->update();

        ImGui::Begin(m_title.c_str(), &m_is_open);

        switch (m_vm->get_mode())
        {
            //--------------------------------------------------------
            // Nothing Selected
            //--------------------------------------------------------
        case inspector_mode::none:
        {
            ImGui::TextDisabled("Nothing selected.");
            break;
        }

        //--------------------------------------------------------
        // Entity Inspector
        //--------------------------------------------------------
        case inspector_mode::entity:
        {
            if (ImGui::Button("Add Component"))
                ImGui::OpenPopup("AddComponentPopup");

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                for (const auto& component : m_vm->get_addable_components())
                {
                    if (ImGui::MenuItem(component.info->display_name.c_str()))
                    {
                        m_vm->request_add_component(component.hash);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            for (const auto& component : m_vm->get_components())
            {
                ImGui::PushID(static_cast<int>(component.hash));

                // ImGui Hilesi: Remove butonunun Header ile aynı satıra düzgünce basılması 
                // ve çökme/çizim bugı yapmaması için hiyerarşiyi koruyoruz.
                bool opened = ImGui::CollapsingHeader(
                    component.display_name.c_str(),
                    component.default_open ? ImGuiTreeNodeFlags_DefaultOpen : 0
                );

                if (component.removable)
                {
                    // Butonu sağ tarafa hizala
                    ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
                    if (ImGui::SmallButton("Remove"))
                    {
                        m_vm->request_remove_component(component.hash);
                        ImGui::PopID();
                        continue; // Silinen componenti bu kare çizme, döngüyü atlat
                    }
                }

                if (opened && component.ui && component.ui->render_fn)
                {
                    // Yenilenen saf render_fn artık tam istediğin gibi 2 parametre alarak çalışıyor!
                    component.ui->render_fn(
                        m_vm->get_selected_entity(),
                        m_vm->get_registry()
                    );
                }

                ImGui::PopID();
            }

            break;
        }

        //--------------------------------------------------------
        // Asset Inspector
        //--------------------------------------------------------
        case inspector_mode::asset:
        {
            auto asset = m_vm->get_asset_data();

            if (!asset.ui)
            {
                ImGui::TextDisabled("No inspector registered.");
                break;
            }

            asset.ui->render_fn(asset.asset_path);
            break;
        }
        }

        ImGui::End();
    }

    inspector_panel::~inspector_panel() = default; 

}