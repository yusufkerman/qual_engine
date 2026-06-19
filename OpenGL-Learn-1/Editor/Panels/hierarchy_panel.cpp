#include <imgui.h>

#include "hierarchy_panel.h"

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
        reg.add_transform(e, {});
        reg.add_name(e, { "New Entity" });

        selectedEntity = (int)e;
        renamingEntity = (int)e;
        strcpy_s(renameBuffer, 128, "New Entity");
    }

    ImGui::Separator();

    for (auto e : reg.get_all_entities())
    {
        std::string name = reg.has_name(e) ? reg.get_name(e)->name : "Entity";
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