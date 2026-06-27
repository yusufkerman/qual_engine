#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>

inline void render_rigidbody_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    if (ImGui::CollapsingHeader("Rigidbody (Jolt Physics)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto* rb = reg.get_component<rigidbody_component>(e);
        if (!rb) return;

        // Jolt BodyID kontrolü
        if (!rb->body_id.IsInvalid()) {
            ImGui::Text("Jolt Body ID: %u", rb->body_id.GetIndex());
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Editor Mode (Fizik Pasif)");
        }

        ImGui::Separator();

        // 1. Motion Type Seçimi
        const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };
        int motionIndex = (int)rb->motion;

        if (ImGui::Combo("Motion Type", &motionIndex, motionNames, 3)) {
            rb->motion = (motion_type)motionIndex;
        }

        // 2. Kütle ve Sürtünme Ayarları
        if (rb->motion == motion_type::static_body) {
            ImGui::BeginDisabled();
            float dummyMass = 0.0f;
            ImGui::DragFloat("Mass (Kg)", &dummyMass, 0.0f);
            ImGui::EndDisabled();
        }
        else {
            ImGui::DragFloat("Mass (Kg)", &rb->mass, 0.05f, 0.01f, 10000.0f, "%.2f");
        }

        ImGui::SliderFloat("Friction", &rb->friction, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // Bileşen Kaldırma Butonu
        if (ImGui::Button("Remove Rigidbody")) {
            if (engineCtx.editor.mode == EngineMode::PLAY && !rb->body_id.IsInvalid()) {
                auto* jolt_system = engineCtx.physicsSystem.get_jolt_system();
                jolt_system->GetBodyInterface().RemoveBody(rb->body_id);
                jolt_system->GetBodyInterface().DestroyBody(rb->body_id);
            }
            reg.remove_component<rigidbody_component>(e);
        }
    }
}

inline void add_rigidbody_component(entity e, registry& reg, EngineContext& engineCtx, EditorContext&) {
    if (!reg.has_component<rigidbody_component>(e)) {
        // Orijinal kodundaki varsayılan değerlerle başlatma
        reg.add_component<rigidbody_component>(e, rigidbody_component{ motion_type::dynamic_body, 1.0f, 0.5f });
    }
}

inline void register_rigidbody_ui_system() {
    ComponentUIInfo info;
    info.name = "Rigidbody";
    info.render_fn = render_rigidbody_ui;
    info.add_fn = add_rigidbody_component;
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<rigidbody_component>(e);
        };

    component_ui_registry::register_component(typeid(rigidbody_component).hash_code(), info);
}