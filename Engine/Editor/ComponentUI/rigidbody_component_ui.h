/** * @file rigidbody_component_ui.h * @brief Katı cisim bileşeni (rigidbody_component) için Jolt Physics tabanlı editör arayüzü. */
#pragma once

#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>

 /** * @brief Seçili varlığın (entity) katı cisim (rigidbody) verilerini ImGui üzerinde çizip günceller.  */
inline void render_rigidbody_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    auto* rb = reg.get_component<rigidbody_component>(e);
    if (!rb) return;

    // ---------------------------------------------------------
    // JOLT PHYSICS BODY ID VE DURUM KONTROLÜ
    // ---------------------------------------------------------
    if (!rb->body_id.IsInvalid()) {
        ImGui::Text("Jolt Body ID: %u", rb->body_id.GetIndex());
    }
    else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Editor Mode (Fizik Pasif)");
    }

    ImGui::Separator();

    // ---------------------------------------------------------
    // 1. HAREKET TÜRÜ SEÇİMİ (MOTION TYPE)
    // ---------------------------------------------------------
    const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };
    int motionIndex = (int)rb->motion;

    if (ImGui::Combo("Motion Type", &motionIndex, motionNames, 3)) {
        rb->motion = (motion_type)motionIndex;
    }

    // ---------------------------------------------------------
    // 2. KÜTLE, SÜRTÜNME VE ESNEKLİK AYARLARI
    // ---------------------------------------------------------
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
    ImGui::SliderFloat("Restitution", &rb->restitution, 0.0f, 1.0f, "%.2f");

    ImGui::Separator();

    // ---------------------------------------------------------
    // 🎯 3. UNITY-LIKE FREEZE ROTATION (KONTROLLERİ YAN YANA DİZME)
    // ---------------------------------------------------------
    ImGui::Text("Constraints");
    ImGui::Text("Freeze Rotation:");
    ImGui::SameLine();

    // Unity tarzı X, Y, Z yan yana dizilim
    if (ImGui::Checkbox("X", &rb->freeze_rotation_x)) {
        // 💡 Eğer runtime modundaysan Jolt'a anlık bildirmek istersen buraya fonksiyon ekleyebilirsin
    }
    ImGui::SameLine();

    if (ImGui::Checkbox("Y", &rb->freeze_rotation_y)) {
        // Jolt anlık güncelleme köprüsü için alan
    }
    ImGui::SameLine();

    if (ImGui::Checkbox("Z", &rb->freeze_rotation_z)) {
        // Jolt anlık güncelleme köprüsü için alan
    }

    ImGui::Separator();

    // ---------------------------------------------------------
    // 4. BİLEŞEN KALDIRMA VE RUTİN FİZİK TEMİZLİĞİ
    // ---------------------------------------------------------
    if (ImGui::Button("Remove Rigidbody")) {
        if (engineCtx.editor.mode == EngineMode::PLAY && !rb->body_id.IsInvalid()) {
            auto* jolt_system = engineCtx.physicsSystem.get_jolt_system();
            jolt_system->GetBodyInterface().RemoveBody(rb->body_id);
            jolt_system->GetBodyInterface().DestroyBody(rb->body_id);
        }
        reg.remove_component<rigidbody_component>(e);
    }
}

inline void add_rigidbody_component(entity e, registry& reg, EngineContext& engineCtx, EditorContext&) {
    if (!reg.has_component<rigidbody_component>(e)) {
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

    // 💡 GÜVENLİ KALDIRMA LAMBDASI: Sağ tık menüsünden veya hiyerarşiden silinirse sızıntıyı önler
    info.remove_fn = [](entity e, registry& reg) {
        // Not: Eğer bu fonksiyonun içinde EngineContext'e erişimin varsa Jolt dünyasından da uçurabilirsin.
        // Şimdilik çökmemesi için ECS üzerinden temizliği koruyoruz:
        reg.remove_component<rigidbody_component>(e);
        };

    component_ui_registry::register_component(typeid(rigidbody_component).hash_code(), info);
}