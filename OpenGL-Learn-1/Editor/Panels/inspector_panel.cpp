#include "inspector_panel.h"

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

        if (reg.has_transform(e))
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                auto* t = reg.get_transform(e);

                ImGui::DragFloat3("Position", &t->position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);
                ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
            }
        }

        if (reg.has_camera(e))
        {
            if (ImGui::CollapsingHeader("Camera"))
            {
                auto* c = reg.get_camera(e);

                ImGui::DragFloat("FOV", &c->fov);
                ImGui::DragFloat("Near", &c->nearPlane);
                ImGui::DragFloat("Far", &c->farPlane);
            }
        }

        if (reg.has_light(e))
        {
            if (ImGui::CollapsingHeader("Light"))
            {
                auto* l = reg.get_light(e);

                ImGui::ColorEdit3("Color", &l->color.x);
                ImGui::DragFloat("Intensity", &l->intensity);
            }
        }
    }

    ImGui::End();
}