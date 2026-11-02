#pragma once

/**
 * @file material_inspector.h
 * @brief Materyal varlıkları (.json, .amat, .mat) için modern PBR ve Retro Shader Inspector paneli.
 * @details Canlı in-memory hot-reload desteği sunar: Slider hareketleri doğrudan bellekteki materyale
 * yansır ve sahnede sıfır gecikmeyle önizlenir. Kalıcı dosya yazma ise yalnızca Save tıklandığında gerçekleşir.
 */

#include "../../Services/asset_ui_registry.h"
#include "../../Services/material_service.h"
#include "../../../Engine/asset_pipeline.h"
#include "../../../Externals/uuid/uuid.h"
#include "../../../Resources/engine_resources.h"
#include "../../Services/asset_entry.h"
#include "../../../Core/path_utils.h"

#include <imgui.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cmath>

namespace vesta {

    namespace detail {

        inline std::string mat_read_file(const std::string& path) {
            std::ifstream f(qual::path_from_utf8(path));
            if (!f.is_open()) return "";
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }

        inline void mat_parse_float_array(const std::string& content, const std::string& key, float* out, int count) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return;

            size_t sb = content.find("[", pos);
            if (sb == std::string::npos) return;

            size_t eb = content.find("]", sb);
            if (eb == std::string::npos || eb <= sb) return;

            std::stringstream ss(content.substr(sb + 1, eb - sb - 1));
            std::string tok;
            int i = 0;
            while (std::getline(ss, tok, ',') && i < count) {
                try {
                    out[i] = std::stof(tok);
                }
                catch (...) {}
                i++;
            }
        }

        inline float mat_parse_float(const std::string& content, const std::string& key, float def) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return def;

            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return def;

            size_t end_pos = content.find_first_of(",}\n\r", colon + 1);
            if (end_pos == std::string::npos) end_pos = content.size();

            std::string val_str = content.substr(colon + 1, end_pos - (colon + 1));
            try { return std::stof(val_str); }
            catch (...) { return def; }
        }

        inline int mat_parse_int(const std::string& content, const std::string& key, int def) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return def;

            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return def;

            size_t end_pos = content.find_first_of(",}\n\r", colon + 1);
            if (end_pos == std::string::npos) end_pos = content.size();

            std::string val_str = content.substr(colon + 1, end_pos - (colon + 1));
            try { return std::stoi(val_str); }
            catch (...) { return def; }
        }

        inline std::string mat_parse_string(const std::string& content, const std::string& key, const std::string& def) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return def;

            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return def;

            size_t q1 = content.find("\"", colon);
            if (q1 == std::string::npos) return def;

            size_t q2 = content.find("\"", q1 + 1);
            if (q2 == std::string::npos) return def;

            return content.substr(q1 + 1, q2 - q1 - 1);
        }

        inline std::string mat_resolve_guid_path(const std::string& guid_str, aria::system* aria_sys) {
            if (guid_str.empty() || guid_str == "00000000-0000-0000-0000-000000000000") return "";
            auto parsed = uuids::uuid::from_string(guid_str);
            if (!parsed) return "";

            if (aria_sys) {
                std::string path = aria_sys->resolve_asset_path(*parsed);
                if (!path.empty()) return path;
            }

            if (engine_resources::is_builtin_guid(*parsed)) {
                return engine_resources::get_builtin_virtual_path_for_guid(*parsed);
            }

            return "";
        }

    } // namespace detail

    /**
     * @brief Materyal düzenleme arayüzünü asset_ui_registry sistemine kaydeder.
     */
    inline void register_material_asset_ui_system(aria::system* aria_sys, material_service* mat_service) {

        AssetUIInfo info;
        info.name = "Material";

        info.can_inspect_fn = [](const std::string& asset_path) -> bool {
            std::string ext = qual::path_from_utf8(asset_path).extension().string();
            return ext == ".json" || ext == ".amat" || ext == ".mat";
        };

        info.render_fn = [aria_sys, mat_service](const std::string& asset_path) {

            // UI tampon durumu
            static std::string last_loaded_path = "";
            static float base_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            static float metallic = 0.0f;
            static float roughness = 0.5f;
            static float ao = 1.0f;
            static float emissive_color[3] = { 0.0f, 0.0f, 0.0f };
            static float emissive_intensity = 0.0f;
            static float normal_strength = 1.0f;
            static float texture_scale[2] = { 1.0f, 1.0f };
            static int render_mode = 0;
            static float alpha_threshold = 0.1f;

            static char shader_path[256] = "";
            static char diffuse_texture_path[256] = "";
            static char normal_texture_path[256] = "";
            static char metallic_texture_path[256] = "";
            static char roughness_texture_path[256] = "";
            static char ao_texture_path[256] = "";
            static char emissive_texture_path[256] = "";
            static char specular_texture_path[256] = "";

            static float save_feedback_time = 0.0f;

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "PBR Material Inspector");
            std::string configFileName = qual::path_to_utf8(qual::path_from_utf8(asset_path).filename());
            ImGui::Text("Config File: %s", configFileName.c_str());
            ImGui::Separator();

            // Dosya ilk açıldığında veya değiştiğinde diskteki güncel JSON verilerini tamponlara oku
            if (last_loaded_path != asset_path) {
                std::string content = detail::mat_read_file(asset_path);
                if (!content.empty()) {
                    detail::mat_parse_float_array(content, "base_color", base_color, 4);
                    metallic = detail::mat_parse_float(content, "metallic", 0.0f);

                    if (content.find("\"roughness\"") != std::string::npos) {
                        roughness = detail::mat_parse_float(content, "roughness", 0.5f);
                    }
                    else {
                        float shininess = detail::mat_parse_float(content, "shininess", 32.0f);
                        roughness = std::clamp(1.0f - std::sqrt(shininess / 256.0f), 0.05f, 1.0f);
                    }

                    ao = detail::mat_parse_float(content, "ao", 1.0f);
                    detail::mat_parse_float_array(content, "emissive_color", emissive_color, 3);
                    emissive_intensity = detail::mat_parse_float(content, "emissive_intensity", 0.0f);
                    normal_strength = detail::mat_parse_float(content, "normal_strength", 1.0f);

                    texture_scale[0] = 1.0f; 
                    texture_scale[1] = 1.0f;
                    detail::mat_parse_float_array(content, "texture_scale", texture_scale, 2);

                    render_mode = detail::mat_parse_int(content, "render_mode", 0);
                    alpha_threshold = detail::mat_parse_float(content, "alpha_threshold", 0.1f);

                    std::string shader_guid = detail::mat_parse_string(content, "shader_guid", "");
                    std::string diffuse_guid = detail::mat_parse_string(content, "diffuse_texture_guid", "");
                    std::string normal_guid = detail::mat_parse_string(content, "normal_texture_guid", "");
                    std::string metallic_guid = detail::mat_parse_string(content, "metallic_texture_guid", "");
                    std::string roughness_guid = detail::mat_parse_string(content, "roughness_texture_guid", "");
                    std::string ao_guid = detail::mat_parse_string(content, "ao_texture_guid", "");
                    std::string emissive_guid = detail::mat_parse_string(content, "emissive_texture_guid", "");
                    std::string specular_guid = detail::mat_parse_string(content, "specular_texture_guid", "");

                    snprintf(shader_path, sizeof(shader_path), "%s", detail::mat_resolve_guid_path(shader_guid, aria_sys).c_str());
                    snprintf(diffuse_texture_path, sizeof(diffuse_texture_path), "%s", detail::mat_resolve_guid_path(diffuse_guid, aria_sys).c_str());
                    snprintf(normal_texture_path, sizeof(normal_texture_path), "%s", detail::mat_resolve_guid_path(normal_guid, aria_sys).c_str());
                    snprintf(metallic_texture_path, sizeof(metallic_texture_path), "%s", detail::mat_resolve_guid_path(metallic_guid, aria_sys).c_str());
                    snprintf(roughness_texture_path, sizeof(roughness_texture_path), "%s", detail::mat_resolve_guid_path(roughness_guid, aria_sys).c_str());
                    snprintf(ao_texture_path, sizeof(ao_texture_path), "%s", detail::mat_resolve_guid_path(ao_guid, aria_sys).c_str());
                    snprintf(emissive_texture_path, sizeof(emissive_texture_path), "%s", detail::mat_resolve_guid_path(emissive_guid, aria_sys).c_str());
                    snprintf(specular_texture_path, sizeof(specular_texture_path), "%s", detail::mat_resolve_guid_path(specular_guid, aria_sys).c_str());

                    if (std::strlen(shader_path) == 0) {
                        snprintf(shader_path, sizeof(shader_path), "%s", detail::mat_parse_string(content, "shader_path", "@engine/Shaders/default_shader").c_str());
                    }
                }
                last_loaded_path = asset_path;
            }

            // Canlı Materyal Bellek Pointer'ı (Hot Reload In-Memory Preview)
            aria::material_asset* live_mat = nullptr;
            aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(asset_path);
            if (aria_sys && !guid.is_nil()) {
                aria_sys->register_asset_path(guid, asset_path);
                auto mat_handle = aria_sys->import_material(guid, asset_path);
                live_mat = aria_sys->registry()->get_material(mat_handle);
            }

            // ========================================================
            // 1. RENDER VE SAYDAMLIK MODU
            // ========================================================
            bool changed = false;
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Render Settings");
            ImGui::Separator();

            const char* render_modes[] = { "Opaque (0)", "Cutout / Alpha Test (1)", "Transparent (2)" };
            changed |= ImGui::Combo("Render Mode", &render_mode, render_modes, IM_ARRAYSIZE(render_modes));
            if (render_mode == 1) {
                changed |= ImGui::SliderFloat("Alpha Threshold", &alpha_threshold, 0.0f, 1.0f, "%.2f");
            }
            ImGui::Spacing();

            // ========================================================
            // 2. MODERN PBR YÜZEY AYARLARI
            // ========================================================
            ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.55f, 1.0f), "PBR Metallic / Roughness Properties");
            ImGui::Separator();

            changed |= ImGui::ColorEdit4("Base Color (Albedo)", base_color);
            changed |= ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f, "%.3f");
            changed |= ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f");
            changed |= ImGui::SliderFloat("Ambient Occlusion (AO)", &ao, 0.0f, 1.0f, "%.3f");
            changed |= ImGui::DragFloat("Normal Strength", &normal_strength, 0.01f, 0.0f, 5.0f);
            changed |= ImGui::DragFloat2("Texture Scale (Tiling)", texture_scale, 0.05f, 0.001f, 100.0f);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.35f, 1.0f), "Emissive (Self-Illumination)");
            ImGui::Separator();
            changed |= ImGui::ColorEdit3("Emissive Color", emissive_color);
            changed |= ImGui::DragFloat("Emissive Intensity", &emissive_intensity, 0.05f, 0.0f, 50.0f, "%.2f");

            // ========================================================
            // 3. SHADER PROGRAMI & ARIA DOKU BAĞLANTILARI
            // ========================================================
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Aria Shader & Texture Slots");
            ImGui::Separator();

            int current_shader_idx = 0; // 0: Standard PBR, 1: PS1 Retro Horror, 2: Skinned Mesh PBR, 3: Custom
            std::string cur_shader_str = shader_path;
            if (cur_shader_str.find("ps1_shader") != std::string::npos || cur_shader_str.find("PS1") != std::string::npos) {
                current_shader_idx = 1;
            }
            else if (cur_shader_str.find("skinned_default") != std::string::npos) {
                current_shader_idx = 2;
            }
            else if (cur_shader_str.empty() || cur_shader_str.find("default_shader") != std::string::npos) {
                current_shader_idx = 0;
            }
            else {
                current_shader_idx = 3;
            }

            const char* shader_options[] = {
                "Standard PBR (@engine/Shaders/default_shader)",
                "PS1 Retro Horror (@engine/Shaders/PS1/ps1_shader)",
                "Skinned Mesh PBR (@engine/Shaders/skinned_default)",
                "Custom Shader (Drag & Drop)"
            };

            int selected_shader_idx = current_shader_idx;
            if (ImGui::Combo("Built-in Shaders", &selected_shader_idx, shader_options, IM_ARRAYSIZE(shader_options))) {
                if (selected_shader_idx == 0) {
                    snprintf(shader_path, sizeof(shader_path), "@engine/Shaders/default_shader");
                    changed = true;
                }
                else if (selected_shader_idx == 1) {
                    snprintf(shader_path, sizeof(shader_path), "@engine/Shaders/PS1/ps1_shader");
                    changed = true;
                }
                else if (selected_shader_idx == 2) {
                    snprintf(shader_path, sizeof(shader_path), "@engine/Shaders/skinned_default");
                    changed = true;
                }
                else if (selected_shader_idx == 3) {
                    if (current_shader_idx != 3) {
                        shader_path[0] = '\0';
                        changed = true;
                    }
                }
            }

            if (selected_shader_idx == 1) {
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "-> PS1 Retro Shader Selected (Affine mapping, vertex jitter, 4x4 dither)");
            }

            // Sürükle bırak yardımcı fonksiyonu
            auto renderDropSlot = [&](const char* label, char* pathBuf, size_t maxLen, vesta::asset_payload_type type) {
                std::string currentVal = (std::strlen(pathBuf) > 0) ? pathBuf : "None (Drag Asset Here)";
                bool hasVal = std::strlen(pathBuf) > 0;

                std::string slotId = std::string(label);
                vesta::DrawAssetDropSlot(
                    slotId.c_str(),
                    currentVal,
                    type,
                    [&](const vesta::asset_drag_payload& payload) {
                        snprintf(pathBuf, maxLen, "%s", payload.path);
                        changed = true;
                    }
                );

                if (hasVal) {
                    ImGui::SameLine();
                    std::string clearBtnId = "X##Clear_" + slotId;
                    if (ImGui::Button(clearBtnId.c_str())) {
                        pathBuf[0] = '\0';
                        changed = true;
                    }
                }
            };

            // Custom Shader slotu
            renderDropSlot("Custom Shader:", shader_path, sizeof(shader_path), vesta::asset_payload_type::shader);

            // Doku Slotları
            renderDropSlot("Albedo / Diffuse Texture (.png/.jpg):", diffuse_texture_path, sizeof(diffuse_texture_path), vesta::asset_payload_type::texture);
            renderDropSlot("Normal Map Texture:", normal_texture_path, sizeof(normal_texture_path), vesta::asset_payload_type::texture);
            renderDropSlot("Metallic Map Texture (R channel):", metallic_texture_path, sizeof(metallic_texture_path), vesta::asset_payload_type::texture);
            renderDropSlot("Roughness Map Texture (R channel):", roughness_texture_path, sizeof(roughness_texture_path), vesta::asset_payload_type::texture);
            renderDropSlot("Ambient Occlusion (AO) Texture (R channel):", ao_texture_path, sizeof(ao_texture_path), vesta::asset_payload_type::texture);
            renderDropSlot("Emissive Texture (RGB):", emissive_texture_path, sizeof(emissive_texture_path), vesta::asset_payload_type::texture);

            // ========================================================
            // 4. CANLI IN-MEMORY HOT-RELOAD GÜNCELLEMESİ
            // ========================================================
            if (changed && live_mat) {
                live_mat->mode = static_cast<aria::render_mode>(render_mode);
                live_mat->alpha_threshold = alpha_threshold;
                live_mat->base_color = glm::vec3(base_color[0], base_color[1], base_color[2]);
                live_mat->metallic = metallic;
                live_mat->roughness = roughness;
                live_mat->ao = ao;
                live_mat->emissive_color = glm::vec3(emissive_color[0], emissive_color[1], emissive_color[2]);
                live_mat->emissive_intensity = emissive_intensity;
                live_mat->normal_strength = normal_strength;
                live_mat->texture_scale = glm::vec2(texture_scale[0], texture_scale[1]);

                if (aria_sys) {
                    if (std::strlen(shader_path) > 0) {
                        aria::asset_guid sh_guid = asset_pipeline::get_or_create_meta_guid(shader_path);
                        live_mat->shader_handle = aria_sys->import_shader(sh_guid, shader_path);
                    }
                    else {
                        live_mat->shader_handle.invalidate();
                    }

                    if (std::strlen(diffuse_texture_path) > 0) {
                        aria::asset_guid d_guid = asset_pipeline::get_or_create_meta_guid(diffuse_texture_path);
                        live_mat->diffuse_texture_handle = aria_sys->import_texture(d_guid, diffuse_texture_path);
                    }
                    else {
                        live_mat->diffuse_texture_handle.invalidate();
                    }

                    if (std::strlen(normal_texture_path) > 0) {
                        aria::asset_guid n_guid = asset_pipeline::get_or_create_meta_guid(normal_texture_path);
                        live_mat->normal_texture_handle = aria_sys->import_texture(n_guid, normal_texture_path);
                    }
                    else {
                        live_mat->normal_texture_handle.invalidate();
                    }

                    if (std::strlen(metallic_texture_path) > 0) {
                        aria::asset_guid m_guid = asset_pipeline::get_or_create_meta_guid(metallic_texture_path);
                        live_mat->metallic_texture_handle = aria_sys->import_texture(m_guid, metallic_texture_path);
                    }
                    else {
                        live_mat->metallic_texture_handle.invalidate();
                    }

                    if (std::strlen(roughness_texture_path) > 0) {
                        aria::asset_guid r_guid = asset_pipeline::get_or_create_meta_guid(roughness_texture_path);
                        live_mat->roughness_texture_handle = aria_sys->import_texture(r_guid, roughness_texture_path);
                    }
                    else {
                        live_mat->roughness_texture_handle.invalidate();
                    }

                    if (std::strlen(ao_texture_path) > 0) {
                        aria::asset_guid a_guid = asset_pipeline::get_or_create_meta_guid(ao_texture_path);
                        live_mat->ao_texture_handle = aria_sys->import_texture(a_guid, ao_texture_path);
                    }
                    else {
                        live_mat->ao_texture_handle.invalidate();
                    }

                    if (std::strlen(emissive_texture_path) > 0) {
                        aria::asset_guid e_guid = asset_pipeline::get_or_create_meta_guid(emissive_texture_path);
                        live_mat->emissive_texture_handle = aria_sys->import_texture(e_guid, emissive_texture_path);
                    }
                    else {
                        live_mat->emissive_texture_handle.invalidate();
                    }
                }
            }

            ImGui::Spacing();

            // ========================================================
            // 5. KALICI KAYDETME (SAVE TO DISK)
            // ========================================================
            const bool saveClicked = ImGui::Button("Save Material Configuration", ImVec2(-1, 35));

            if (save_feedback_time > 0.0f) {
                save_feedback_time -= ImGui::GetIO().DeltaTime;
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[SUCCESS] Material configuration saved to disk!");
            }

            if (saveClicked) {
                auto resolveAndRegisterGuid = [aria_sys](const char* pathBuf) -> std::string {
                    if (!pathBuf || std::strlen(pathBuf) == 0) {
                        return "00000000-0000-0000-0000-000000000000";
                    }
                    aria::asset_guid g = asset_pipeline::get_or_create_meta_guid(pathBuf);
                    if (aria_sys && !g.is_nil()) {
                        aria_sys->register_asset_path(g, pathBuf);
                    }
                    return uuids::to_string(g);
                };

                std::string shader_guid_str = resolveAndRegisterGuid(shader_path);
                std::string diffuse_guid_str = resolveAndRegisterGuid(diffuse_texture_path);
                std::string normal_guid_str = resolveAndRegisterGuid(normal_texture_path);
                std::string metallic_guid_str = resolveAndRegisterGuid(metallic_texture_path);
                std::string roughness_guid_str = resolveAndRegisterGuid(roughness_texture_path);
                std::string ao_guid_str = resolveAndRegisterGuid(ao_texture_path);
                std::string emissive_guid_str = resolveAndRegisterGuid(emissive_texture_path);
                std::string specular_guid_str = resolveAndRegisterGuid(specular_texture_path);

                float shininessVal = std::pow(1.0f - roughness, 2.0f) * 256.0f;

                std::ostringstream json;
                json << "{\n"
                    << "  \"type\": \"material\",\n"
                    << "  \"name\": \"" << qual::path_to_utf8(qual::path_from_utf8(asset_path).stem()) << "\",\n"
                    << "  \"base_color\": [" << base_color[0] << ", " << base_color[1] << ", " << base_color[2] << ", " << base_color[3] << "],\n"
                    << "  \"metallic\": " << metallic << ",\n"
                    << "  \"roughness\": " << roughness << ",\n"
                    << "  \"ao\": " << ao << ",\n"
                    << "  \"emissive_color\": [" << emissive_color[0] << ", " << emissive_color[1] << ", " << emissive_color[2] << "],\n"
                    << "  \"emissive_intensity\": " << emissive_intensity << ",\n"
                    << "  \"specular_strength\": 0.5,\n"
                    << "  \"shininess\": " << shininessVal << ",\n"
                    << "  \"normal_strength\": " << normal_strength << ",\n"
                    << "  \"texture_scale\": [" << texture_scale[0] << ", " << texture_scale[1] << "],\n"
                    << "  \"render_mode\": " << render_mode << ",\n"
                    << "  \"alpha_threshold\": " << alpha_threshold << ",\n"
                    << "  \"shader_guid\": \"" << shader_guid_str << "\",\n"
                    << "  \"shader_path\": \"" << qual::path_to_utf8(qual::path_from_utf8(shader_path).generic_string()) << "\",\n"
                    << "  \"diffuse_texture_guid\": \"" << diffuse_guid_str << "\",\n"
                    << "  \"normal_texture_guid\": \"" << normal_guid_str << "\",\n"
                    << "  \"metallic_texture_guid\": \"" << metallic_guid_str << "\",\n"
                    << "  \"roughness_texture_guid\": \"" << roughness_guid_str << "\",\n"
                    << "  \"ao_texture_guid\": \"" << ao_guid_str << "\",\n"
                    << "  \"emissive_texture_guid\": \"" << emissive_guid_str << "\",\n"
                    << "  \"specular_texture_guid\": \"" << specular_guid_str << "\"\n"
                    << "}";

                if (mat_service && !guid.is_nil()) {
                    bool ok = mat_service->save_material_changes(guid, json.str());
                    if (ok) {
                        save_feedback_time = 3.0f;
                        std::cout << "[Vesta MaterialInspector] Materyal basariyla kaydedildi: " << asset_path << std::endl;
                    }
                    else {
                        std::cerr << "[Vesta MaterialInspector] Hata: save_material_changes basarisiz oldu: " << asset_path << std::endl;
                    }
                }
            }
        };

        asset_ui_registry::register_asset("material", info);
    }

} // namespace vesta