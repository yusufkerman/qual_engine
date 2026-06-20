#include "hierarchy_panel.h"
#include "../../ECS/registry.h"
#include <imgui.h>

static std::string getEntityName(registry& reg, entity e)
{
    if (auto* n = reg.get_component<name_component>(e))
        return n->name;

    return "Entity";
}

void hierarchy_panel::render(scene& scene,
    int& selectedEntity,
    int& renamingEntity,
    char* renameBuffer,
    size_t renameBufferSize,
    std::vector<entity>& deleteQueue)
{
    auto& reg = scene.get_registry();

    ImGui::Begin("Hierarchy");

    // CREATE OBJECT
    if (ImGui::Button("Create Object"))
    {
        entity e = scene.create_entity();
        auto& reg = scene.get_registry();

        reg.add_component<transform_component>(e, transform_component{});
        reg.add_component<name_component>(e, name_component{ "New Entity" });

        selectedEntity = (int)e;
        renamingEntity = (int)e;
        strcpy_s(renameBuffer, renameBufferSize, "New Entity");
    }

    ImGui::Separator();

    // ENTITY LIST
    auto entities = reg.get_all_entities(); // COPY

    for (auto e : entities)
    {
        std::string name = getEntityName(reg, e);
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
                    strcpy_s(renameBuffer, renameBufferSize, name.c_str());
                }

                if (ImGui::MenuItem("Delete"))
                {
                    if (selectedEntity == (int)e)
                        selectedEntity = -1;

                    deleteQueue.push_back(e);
                }

                ImGui::EndPopup();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                renamingEntity = (int)e;
                strcpy_s(renameBuffer, renameBufferSize, name.c_str());
            }
        }
        else
        {
            ImGui::SetNextItemWidth(160);

            bool enter = ImGui::InputText(
                "##rename",
                renameBuffer,
                renameBufferSize,
                ImGuiInputTextFlags_EnterReturnsTrue
            );

            ImGui::SameLine();

            bool cancel = ImGui::SmallButton("X");
            bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape);

            if (enter || cancel || esc)
            {
                if (enter)
                {
                    if (auto* n = reg.get_component<name_component>(e))
                        n->name = renameBuffer;
                    else
                        reg.add_component<name_component>(e, name_component{ renameBuffer });
                }

                renamingEntity = -1;
            }
        }

        ImGui::PopID();
    }

    ImGui::End();
}