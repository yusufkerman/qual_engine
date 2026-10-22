#pragma once

/**
 * @file audio_clip_inspector.h
 * @brief Ses varlıkları (.wav, .mp3, .ogg, .flac) için Asset Inspector paneli ve canlı önizleme (preview) sistemi.
 */

#include "../../Services/asset_ui_registry.h"
#include "../../Services/asset_entry.h"
#include "../../../Engine/Assets/aria.h"
#include "../../../Engine/asset_pipeline.h"
#include "../../../Audio/audio_system.h"
#include <imgui.h>
#include <filesystem>
#include <iostream>

namespace vesta {

    /**
     * @brief Ses (Audio Clip) varlık türü için Inspector arayüzünü ve canlı dinleme oynatıcısını kaydeder.
     * @param aria_sys ARIA varlık yönetim sistemi.
     * @param audio_sys Ses sistemi referansı (Editör içi önizleme oynatımı için).
     */
    inline void register_audio_clip_asset_ui_system(aria::system* aria_sys, audio::audio_system* audio_sys) {
        if (!aria_sys) return;

        AssetUIInfo info;
        info.name = "Audio Clip Asset";

        info.can_inspect_fn = [](const std::string& asset_path) -> bool {
            namespace fs = std::filesystem;
            std::string ext = fs::path(asset_path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            return (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac");
        };

        info.render_fn = [aria_sys, audio_sys](const std::string& asset_path) {
            namespace fs = std::filesystem;

            if (!fs::exists(asset_path)) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: File does not exist!");
                ImGui::TextDisabled("%s", asset_path.c_str());
                return;
            }

            fs::path p(asset_path);
            std::string filename = p.filename().string();
            std::string ext = p.extension().string();
            uintmax_t fileSize = 0;
            try { fileSize = fs::file_size(p); } catch (...) {}

            ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "Audio Clip Importer");
            ImGui::Separator();

            ImGui::Text("File: %s", filename.c_str());
            ImGui::TextDisabled("Path: %s", asset_path.c_str());
            ImGui::Text("Size: %.2f KB", static_cast<float>(fileSize) / 1024.0f);

            // ARIA GUID ve Audio Clip handle'ını çöz
            aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(asset_path);
            aria::handle<aria::audio_clip_asset> clipHandle = aria_sys->import_audio(guid, asset_path);
            const aria::audio_clip_asset* clipData = aria_sys->get_audio_data(clipHandle);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Audio Format Details:");
            ImGui::Separator();

            if (clipData) {
                int mins = static_cast<int>(clipData->duration_seconds) / 60;
                int secs = static_cast<int>(clipData->duration_seconds) % 60;
                int millis = static_cast<int>((clipData->duration_seconds - std::floor(clipData->duration_seconds)) * 1000.0f);

                ImGui::Text("Duration: %02d:%02d.%03d (%.2f s)", mins, secs, millis, clipData->duration_seconds);
                ImGui::Text("Channels: %u (%s)", clipData->channels, (clipData->channels == 1) ? "Mono" : (clipData->channels == 2) ? "Stereo" : "Multi-channel");
                ImGui::Text("Sample Rate: %u Hz", clipData->sample_rate);
            }
            else {
                ImGui::TextDisabled("Metadata could not be loaded.");
            }

            ImGui::Spacing();
            ImGui::Separator();

            // ====================================================================
            // EDİTÖR İÇİ CANLI SES ÖNİZLEME (PREVIEW PLAYER)
            // ====================================================================
            static audio::sound_id s_activePreviewId = audio::invalid_sound_id;
            static std::string s_activePreviewPath = "";
            static float s_previewVolume = 1.0f;

            ImGui::TextColored(ImVec4(0.5f, 0.95f, 0.6f, 1.0f), "Editor Audio Preview");

            if (audio_sys && audio_sys->get_backend()) {
                auto* backend = audio_sys->get_backend();

                bool isCurrentPlaying = (s_activePreviewId != audio::invalid_sound_id &&
                                         s_activePreviewPath == asset_path &&
                                         backend->is_sound_playing(s_activePreviewId));

                float currentPos = 0.0f;
                if (s_activePreviewId != audio::invalid_sound_id && s_activePreviewPath == asset_path) {
                    currentPos = backend->get_sound_cursor(s_activePreviewId);
                }

                float totalDuration = clipData ? clipData->duration_seconds : 1.0f;
                float progress = (totalDuration > 0.0f) ? (currentPos / totalDuration) : 0.0f;
                progress = glm::clamp(progress, 0.0f, 1.0f);

                char progressOverlay[64];
                snprintf(progressOverlay, sizeof(progressOverlay), "%.2f s / %.2f s", currentPos, totalDuration);
                ImGui::ProgressBar(progress, ImVec2(-1, 20), progressOverlay);

                ImGui::Spacing();

                if (!isCurrentPlaying) {
                    if (ImGui::Button("Play Preview", ImVec2(120, 26))) {
                        if (s_activePreviewId != audio::invalid_sound_id && s_activePreviewPath != asset_path) {
                            backend->stop_sound(s_activePreviewId);
                            backend->unload_sound(s_activePreviewId);
                            s_activePreviewId = audio::invalid_sound_id;
                        }

                        if (s_activePreviewId == audio::invalid_sound_id) {
                            s_activePreviewId = backend->load_sound(asset_path, false);
                            s_activePreviewPath = asset_path;
                        }

                        if (s_activePreviewId != audio::invalid_sound_id) {
                            backend->set_sound_volume(s_activePreviewId, s_previewVolume);
                            backend->play_sound(s_activePreviewId);
                        }
                    }
                }
                else {
                    if (ImGui::Button("Pause", ImVec2(120, 26))) {
                        backend->pause_sound(s_activePreviewId);
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Stop", ImVec2(90, 26))) {
                    if (s_activePreviewId != audio::invalid_sound_id) {
                        backend->stop_sound(s_activePreviewId);
                        backend->unload_sound(s_activePreviewId);
                        s_activePreviewId = audio::invalid_sound_id;
                        s_activePreviewPath = "";
                    }
                }

                ImGui::Spacing();
                if (ImGui::SliderFloat("Preview Volume", &s_previewVolume, 0.0f, 1.0f, "%.2f")) {
                    if (s_activePreviewId != audio::invalid_sound_id) {
                        backend->set_sound_volume(s_activePreviewId, s_previewVolume);
                    }
                }
            }
            else {
                ImGui::TextDisabled("Audio system is not available for preview.");
            }
        };

        vesta::asset_ui_registry::register_asset("audio", info);
    }

} // namespace vesta

