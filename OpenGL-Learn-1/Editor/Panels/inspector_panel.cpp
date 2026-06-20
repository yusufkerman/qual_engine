#include "inspector_panel.h"
#include "../../ECS/registry.h"

#include <imgui.h>

void inspector_panel::render(scene& scene,
    int selectedEntity)
{
    auto& reg = scene.get_registry();

    ImGui::Begin("Inspector");

    if (selectedEntity == -1)
    {
        ImGui::Text("No entity selected");
        ImGui::End();
        return;
    }

    entity e = (entity)selectedEntity;

    ImGui::Text("Entity %d", selectedEntity);
    ImGui::Separator();

    // =========================
    // ADD COMPONENT BUTTON
    // =========================
    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("add_component_popup");

    if (ImGui::BeginPopup("add_component_popup"))
    {
        if (!reg.has_component<transform_component>(e))
        {
            if (ImGui::MenuItem("Transform"))
                reg.add_component<transform_component>(e);
        }

        if (!reg.has_component<camera_component>(e))
        {
            if (ImGui::MenuItem("Camera"))
                reg.add_component<camera_component>(e);
        }

        if (!reg.has_component<light_component>(e))
        {
            if (ImGui::MenuItem("Light"))
                reg.add_component<light_component>(e);
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();

    // =========================
    // TRANSFORM
    // =========================
    if (reg.has_component<transform_component>(e))
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* t = reg.get_component<transform_component>(e);

            ImGui::DragFloat3("Position", &t->position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);
            ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
        }
    }

    // =========================
    // CAMERA
    // =========================
    if (reg.has_component<camera_component>(e))
    {
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* c = reg.get_component<camera_component>(e);

            ImGui::DragFloat("FOV", &c->fov, 1.0f);
            ImGui::DragFloat("Near", &c->nearPlane, 0.01f);
            ImGui::DragFloat("Far", &c->farPlane, 1.0f);
        }
    }

    // =========================
    // LIGHT
    // =========================
    if (reg.has_component<light_component>(e))
    {
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* l = reg.get_component<light_component>(e);

            ImGui::ColorEdit3("Color", &l->color.x);
            ImGui::DragFloat("Intensity", &l->intensity, 0.1f);
            ImGui::DragFloat("Range", &l->range, 1.0f);
            ImGui::DragFloat("Inner Cone", &l->inner_cone, 0.5f, 0.0f, 89.0f);
            ImGui::DragFloat("Outer Cone", &l->outer_cone, 0.5f, 0.0f, 90.0f);

            const char* typeNames[] = { "Directional", "Point", "Spot" };
            int typeIndex = (int)l->type;
            if (ImGui::Combo("Type", &typeIndex, typeNames, 3))
                l->type = (light_type)typeIndex;
        }
    }

    ImGui::End();
}