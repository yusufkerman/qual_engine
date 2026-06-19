#include "inspector_panel.h"
#include "../../ECS/registry.h"

#include <imgui.h>

void inspector_panel::render(scene& scene,
    int selectedEntity)
{
    auto& reg = scene.get_registry();

    ImGui::Begin("Inspector");

    if (selectedEntity != -1)
    {
        entity e = (entity)selectedEntity;

        ImGui::Text("Entity %d", selectedEntity);

        if (reg.has_component<transform_component>(e))
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                auto* t = reg.get_component<transform_component>(e);

                ImGui::DragFloat3("Position", &t->position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);
                ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
            }
        }

        if (reg.has_component<camera_component>(e))
        {
            if (ImGui::CollapsingHeader("Camera"))
            {
                auto* c = reg.get_component<camera_component>(e);

                ImGui::DragFloat("FOV", &c->fov);
                ImGui::DragFloat("Near", &c->nearPlane);
                ImGui::DragFloat("Far", &c->farPlane);
            }
        }

        if (reg.has_component<light_component>(e))
        {
            if (ImGui::CollapsingHeader("Light"))
            {
                auto* l = reg.get_component<light_component>(e);

                ImGui::ColorEdit3("Color", &l->color.x);
                ImGui::DragFloat("Intensity", &l->intensity);
            }
        }
    }

    ImGui::End();
}