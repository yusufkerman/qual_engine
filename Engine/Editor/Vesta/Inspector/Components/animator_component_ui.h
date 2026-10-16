#pragma once

/**
 * @file animator_component_ui.h
 * @brief Animatör bileşeni (animator_component) için klip seçimi, oynatma kontrolleri ve zaman çubuğu editör arayüzü.
 */

#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../ECS/registry.h"
#include "../../ECS/components.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Services/asset_entry.h"
#include <imgui.h>
#include <iostream>

inline void render_animator_ui(entity e, registry& reg, aria::system& ariaSys) {
    auto* anim = reg.get_component<animator_component>(e);
    if (!anim) return;

    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Animator Component");
    ImGui::Separator();

    std::string clipName = "Klip Yok (Animasyon Dosyasi Surukleyin)";
    float durationSec = 0.0f;
    if (anim->current_clip.is_valid()) {
        const auto* clipData = ariaSys.get_animation_data(anim->current_clip);
        if (clipData) {
            float tps = clipData->raw_data.ticks_per_second > 0.0f ? clipData->raw_data.ticks_per_second : 25.0f;
            durationSec = clipData->raw_data.duration / tps;

            // Dosya yolundan gerçek dosya adını ayıkla (Örn: "Capoeira.fbx")
            if (!clipData->raw_data.debug_path.empty()) {
                std::filesystem::path p(clipData->raw_data.debug_path);
                clipName = p.filename().string();
            } else if (!clipData->raw_data.name.empty() && clipData->raw_data.name != "mixamo.com") {
                clipName = clipData->raw_data.name;
            } else {
                clipName = "Animasyon Klibi";
            }
        }
    }

    ImGui::Text("Aktif Klip: %s", clipName.c_str());

    // Sürükle-bırak animasyon dosyası kabulü (.fbx)
    ImGui::Button(clipName.c_str(), ImVec2(-1, 30));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
            auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);
            if (data && data->type == vesta::asset_payload_type::model) {
                aria::handle<aria::animation_clip_asset> clipHandle =
                    asset_pipeline::import_animation(&ariaSys, data->path);
                if (clipHandle.is_valid()) {
                    anim->play(clipHandle, anim->is_looping);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Spacing();

    // Zaman ve İlerleme Çubuğu
    if (durationSec > 0.0f) {
        float progress = anim->playback_time / durationSec;
        char buf[64];
        snprintf(buf, sizeof(buf), "%.2f s / %.2f s", anim->playback_time, durationSec);
        if (ImGui::SliderFloat("Time", &anim->playback_time, 0.0f, durationSec, buf)) {
            // Kullanıcı scrubber'ı kaydırdığında skeleton'ı dirty işaretle
            auto* skel = reg.get_component<skeleton_component>(e);
            if (skel) skel->is_dirty = true;
        }
    }

    // Kontrol Düğmeleri (Play / Pause / Stop)
    if (anim->is_playing) {
        if (ImGui::Button("Pause", ImVec2(70, 25))) {
            anim->pause();
        }
    } else {
        if (ImGui::Button("Play", ImVec2(70, 25))) {
            anim->resume();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(70, 25))) {
        anim->stop();
        auto* skel = reg.get_component<skeleton_component>(e);
        if (skel) skel->is_dirty = true;
    }

    ImGui::Spacing();

    // Parametreler
    ImGui::SliderFloat("Speed", &anim->playback_speed, -3.0f, 3.0f, "%.2fx");
    ImGui::Checkbox("Looping", &anim->is_looping);
    ImGui::Checkbox("Root Motion", &anim->apply_root_motion);

    if (anim->blend_state.is_blending) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Crossfading: %.1f%%", anim->blend_state.current_weight * 100.0f);
    }
}

inline void register_animator_ui_system(aria::system& ariaSys) {
    size_t comp_hash = typeid(animator_component).hash_code();

    vesta::ComponentOperationInfo logic;
    logic.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<animator_component>(e)) {
            reg.add_component<animator_component>(e);
        }
    };
    logic.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<animator_component>(e);
    };
    logic.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<animator_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, logic);

    vesta::ComponentUIInfo ui;
    ui.display_name = "Animator";
    ui.render_fn = [&ariaSys](entity e, registry& reg) {
        render_animator_ui(e, reg, ariaSys);
    };
    vesta::component_ui_registry::register_component(comp_hash, ui);
}

