#include <imgui.h>

#include "hierarchy_panel.h"
#include "../../ECS/registry.h"


void hierarchy_panel::render(scene& scene,
    int& selectedEntity,
    int& renamingEntity,
    char* renameBuffer,
    std::vector<entity>& deleteQueue)
{
    auto& reg = scene.get_registry();

    ImGui::Begin("Hierarchy");

    if (ImGui::Button("Create Object"))
    {
        entity e = reg.create_entity();
        reg.add_component<transform_component>(e);
        reg.add_component<name_component>(e);

        selectedEntity = (int)e;
        renamingEntity = (int)e;
        strcpy_s(renameBuffer, 128, "New Entity");
    }

    ImGui::Separator();

    for (auto e : reg.get_all_entities())
    {
        std::string name = reg.has_component<name_component>(e) ? reg.get_component<name_component>(e)->name : "Entity";
        bool isSelected = (selectedEntity == (int)e);

        ImGui::PushID((int)e);

        if (renamingEntity != (int)e)
        {
            if (ImGui::Selectable(name.c_str(), isSelected))
                selectedEntity = (int)e;

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    renamingEntity = (int)e;
                    strcpy_s(renameBuffer, 128, name.c_str());
                }

                if (ImGui::MenuItem("Delete"))
                {
                    if (selectedEntity == (int)e)
                        selectedEntity = -1;

                    deleteQueue.push_back(e);
                }

                ImGui::EndPopup();
            }
        }

        ImGui::PopID();
    }

    ImGui::End();
}