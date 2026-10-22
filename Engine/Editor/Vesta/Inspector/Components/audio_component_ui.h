#pragma once

/**
 * @file audio_component_ui.h
 * @brief audio_source_component ve audio_listener_component için Vesta Inspector arayüzü ve sürükle-bırak slotları.
 */

#include "../../Services/component_operation_registry.h"
#include "../../Services/component_ui_registry.h"
#include "../../../UIHelper.h"
#include "../../../ECS/registry.h"
#include "../../../Engine/Assets/aria.h"
#include "../../../Engine/asset_pipeline.h"
#include "../../../Audio/audio_components.h"
#include "../../../Audio/audio_system.h"
#include <imgui.h>
#include <filesystem>
#include <iostream>

namespace vesta {

    inline void render_audio_source_ui(entity e, registry& reg, aria::system& ariaSys, audio::audio_system& audioSys)
    {
        auto* src = reg.get_component<audio_source_component>(e);
        if (!src) return;

        // Dosya adı etiketi
        std::string displayLabel = "None (Drag Audio Here)";
        if (!src->clip_path.empty()) {
            displayLabel = std::filesystem::path(src->clip_path).filename().string();
        }

        // 1. Sürükle - Bırak (Drag & Drop) Slotu
        vesta::DrawAssetDropSlot(
            "Audio Clip",
            displayLabel,
            vesta::asset_payload_type::audio,
            [&](const vesta::asset_drag_payload& payload)
            {
                src->clip_path = payload.path;
                src->clip_guid = payload.guid;

                // Eski ses örneğini durdur ve sıfırla
                if (src->runtime_sound_id != audio::invalid_sound_id) {
                    audioSys.stop_source(e, reg);
                    if (audioSys.get_backend()) {
                        audioSys.get_backend()->unload_sound(src->runtime_sound_id);
                    }
                    src->runtime_sound_id = audio::invalid_sound_id;
                }
            }
        );

        if (!src->clip_path.empty()) {
            ImGui::TextDisabled("File: %s", src->clip_path.c_str());
        }

        ImGui::Spacing();

        // 2. Ses Ayarları
        if (ImGui::SliderFloat("Volume", &src->volume, 0.0f, 2.0f, "%.2f")) {
            if (src->runtime_sound_id != audio::invalid_sound_id && audioSys.get_backend()) {
                audioSys.get_backend()->set_sound_volume(src->runtime_sound_id, src->volume);
            }
        }
        if (ImGui::SliderFloat("Pitch", &src->pitch, 0.1f, 3.0f, "%.2f")) {
            if (src->runtime_sound_id != audio::invalid_sound_id && audioSys.get_backend()) {
                audioSys.get_backend()->set_sound_pitch(src->runtime_sound_id, src->pitch);
            }
        }
        if (ImGui::Checkbox("Loop", &src->loop)) {
            if (src->runtime_sound_id != audio::invalid_sound_id && audioSys.get_backend()) {
                audioSys.get_backend()->set_sound_looping(src->runtime_sound_id, src->loop);
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Play On Awake", &src->play_on_awake);

        if (ImGui::Checkbox("3D Spatial Audio", &src->is_3d)) {
            if (src->runtime_sound_id != audio::invalid_sound_id && audioSys.get_backend()) {
                audioSys.get_backend()->set_sound_spatialization_enabled(src->runtime_sound_id, src->is_3d);
            }
        }

        if (src->is_3d) {
            ImGui::Spacing();
            ImGui::TextDisabled("3D Spatial Attenuation (Linear Model):");
            if (ImGui::DragFloat("Min Distance", &src->min_distance, 0.5f, 0.1f, 1000.0f, "%.1f m") ||
                ImGui::DragFloat("Max Distance", &src->max_distance, 1.0f, 0.5f, 5000.0f, "%.1f m") ||
                ImGui::SliderFloat("Roll-off Factor", &src->roll_off, 0.0f, 5.0f, "%.2f"))
            {
                if (src->runtime_sound_id != audio::invalid_sound_id && audioSys.get_backend()) {
                    audioSys.get_backend()->set_sound_attenuation(src->runtime_sound_id, src->min_distance, src->max_distance, src->roll_off);
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 3. Editör Test / Önizleme Kontrolleri
        ImGui::TextDisabled("Editor Preview:");
        const bool isPlaying = src->is_playing;

        if (!isPlaying) {
            if (ImGui::Button("Play (3D)", ImVec2(90, 24))) {
                audioSys.play_source(e, reg, false);
            }
            ImGui::SameLine();
            if (ImGui::Button("Preview (2D)", ImVec2(100, 24))) {
                audioSys.play_source(e, reg, true /* force2DPreview */);
            }
        }
        else {
            if (ImGui::Button("Pause", ImVec2(90, 24))) {
                audioSys.pause_source(e, reg);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Stop", ImVec2(70, 24))) {
            audioSys.stop_source(e, reg);
        }

        ImGui::SameLine();
        if (isPlaying) {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Playing...");
        }
        else if (src->is_paused) {
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Paused");
        }
        else {
            ImGui::TextDisabled("Stopped");
        }
    }

    inline void render_audio_listener_ui(entity e, registry& reg)
    {
        auto* listener = reg.get_component<audio_listener_component>(e);
        if (!listener) return;

        ImGui::Checkbox("Is Active Listener", &listener->is_active);
        ImGui::TextWrapped("Receives 3D spatial audio in the scene. Position and orientation are automatically synchronized from this entity's Transform Component.");
    }

    inline void register_audio_components_ui_system(aria::system& ariaSys, audio::audio_system& audioSys)
    {
        // ==========================================
        // 1. AUDIO SOURCE COMPONENT
        // ==========================================
        size_t source_hash = typeid(audio_source_component).hash_code();

        vesta::ComponentOperationInfo source_logic;
        source_logic.add_fn = [](entity e, registry& reg) {
            if (!reg.has_component<audio_source_component>(e)) {
                reg.add_component<audio_source_component>(e);
            }
        };
        source_logic.has_fn = [](entity e, registry& reg) -> bool {
            return reg.has_component<audio_source_component>(e);
        };
        source_logic.remove_fn = [&audioSys](entity e, registry& reg) {
            audioSys.stop_source(e, reg);
            auto* src = reg.get_component<audio_source_component>(e);
            if (src && src->runtime_sound_id != audio::invalid_sound_id) {
                if (audioSys.get_backend()) {
                    audioSys.get_backend()->unload_sound(src->runtime_sound_id);
                }
            }
            reg.remove_component<audio_source_component>(e);
        };
        vesta::component_operation_registry::register_component(source_hash, source_logic);

        vesta::ComponentUIInfo source_ui;
        source_ui.display_name = "Audio Source";
        source_ui.category = "Audio";
        source_ui.render_fn = [&ariaSys, &audioSys](entity e, registry& reg) {
            render_audio_source_ui(e, reg, ariaSys, audioSys);
        };
        vesta::component_ui_registry::register_component(source_hash, source_ui);

        // ==========================================
        // 2. AUDIO LISTENER COMPONENT
        // ==========================================
        size_t listener_hash = typeid(audio_listener_component).hash_code();

        vesta::ComponentOperationInfo listener_logic;
        listener_logic.add_fn = [](entity e, registry& reg) {
            if (!reg.has_component<audio_listener_component>(e)) {
                reg.add_component<audio_listener_component>(e);
            }
        };
        listener_logic.has_fn = [](entity e, registry& reg) -> bool {
            return reg.has_component<audio_listener_component>(e);
        };
        listener_logic.remove_fn = [](entity e, registry& reg) {
            reg.remove_component<audio_listener_component>(e);
        };
        vesta::component_operation_registry::register_component(listener_hash, listener_logic);

        vesta::ComponentUIInfo listener_ui;
        listener_ui.display_name = "Audio Listener";
        listener_ui.category = "Audio";
        listener_ui.render_fn = [](entity e, registry& reg) {
            render_audio_listener_ui(e, reg);
        };
        vesta::component_ui_registry::register_component(listener_hash, listener_ui);
    }

} // namespace vesta

