#pragma once

#include "../Vesta/Services/component_ui_registry.h"
#include "../Vesta/Services/component_operation_registry.h"

#include "../../ECS/registry.h"
#include <imgui.h>


inline void render_transform_ui(entity e, registry& reg)
{
    auto* t = reg.get_component<transform_component>(e);

    if (t)
    {
        ImGui::DragFloat3("Position", &t->position.x, 0.1f);
        ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);
        ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
    }
}


inline void register_transform_ui_system()
{
    size_t comp_hash = typeid(transform_component).hash_code();


    // ==========================================
    // 1. COMPONENT UI REGISTRY
    // ==========================================

    vesta::ComponentUIInfo ui_info;

    ui_info.display_name = "Transform";
    ui_info.render_fn = render_transform_ui;

    vesta::component_ui_registry::register_component(
        comp_hash,
        ui_info
    );


    // ==========================================
    // 2. COMPONENT OPERATION REGISTRY
    // ==========================================

    vesta::ComponentOperationInfo op_info;


    op_info.add_fn =
        [](entity e, registry& reg)
        {
            if (!reg.has_component<transform_component>(e))
            {
                reg.add_component<transform_component>(e);
            }
        };


    op_info.has_fn =
        [](entity e, registry& reg)
        {
            return reg.has_component<transform_component>(e);
        };


    op_info.remove_fn =
        [](entity e, registry& reg)
        {
            reg.remove_component<transform_component>(e);
        };


    vesta::component_operation_registry::register_component(
        comp_hash,
        op_info
    );
}