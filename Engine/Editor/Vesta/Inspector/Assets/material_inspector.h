#pragma once

/**
 * @file material_asset_ui.h
 * @brief Materyal varlıkları (.json, .amat) için modern PBR (Metallic-Roughness) Inspector paneli
 * arayüz kaydını, sürükle-bırak slotlarını ve çizim mantığını barındırır.
 * @note Dış servis bağımlılıkları (Aria ve MaterialService) kayıt anında closure içerisine yakalanır.
 */

#include "../../Services/asset_ui_registry.h"
#include "../../Services/material_service.h"
#include "../../../Engine/asset_pipeline.h"
#include "../../../Externals/uuid/uuid.h"
#include "../../../Resources/engine_resources.h"
#include "../../Services/asset_entry.h"

#include <imgui.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cmath>

namespace vesta {

    namespace detail {

        /**
         * @brief Verilen dosya yolundaki içeriği okuyup string olarak döndürür.
         */
        inline std::string mat_read_file(const std::string& path) {
            std::ifstream f(path);
            if (!f.is_open()) return "";
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }

        /**
         * @brief JSON içeriğinden belirtilen anahtara ait float dizisini ayrıştırır.
         */
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

        /**
         * @brief JSON içeriğinden float değer ayrıştırır.
         */
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

        /**
         * @brief JSON içeriğinden integer değer ayrıştırır.
         */
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

        /**
         * @brief JSON içeriğinden tırnak içerisindeki string değerini ayrıştırır.
         */
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

        /**
         * @brief String GUID verisini Aria varlık sistemi üzerinden fiziksel dosya yoluna dönüştürür.
         */
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
     * 
     * @param aria_sys GUID çözümleme işlemleri için Aria varlık sistemi referansı.
     * @param mat_service Materyal kaydetme işlemlerini yönetecek servis referansı.
     */
    inline void register_material_asset_ui_system(aria::system* aria_sys, material_service* mat_service) {

        AssetUIInfo info;
        info.name = "Material";

        info.can_inspect_fn = [](const std::string& asset_path) -> bool {
            std::string ext = std::filesystem::path(asset_path).extension().string();
            return ext == ".json" || ext == ".amat";
        };

        info.render_fn = [aria_sys, mat_service](const std::string& asset_path) {

            // UI çizim durumu (Panel her çizildiğinde korunur)
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

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "PBR Material Inspector");
            ImGui::Text("Config File: %s", std::filesystem::path(asset_path).filename().string().c_str());
            ImGui::Separator();

            // Dosya değiştiğinde diskteki güncel JSON verilerini tamponlara oku
            if (last_loaded_path != asset_path) {
                std::string content = detail::mat_read_file(asset_path);
                if (!content.empty()) {
                    detail::mat_parse_float_array(content, "base_color", base_color, 4);
                    metallic = detail::mat_parse_float(content, "metallic", 0.0f);

                    if (content.find("\"roughness\"") != std::string::npos) {
                        roughness = detail::mat_parse_float(content, "roughness", 0.5f);
                    }
                    else {
                        // Geriye dönük uyumluluk: shininess'tan roughness türet
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
            // 3. ARIA DOKU VE SHADER BAĞLANTILARI (DRAG & DROP)
            // ========================================================
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.85f, 1.0f, 1.0f), "Aria Texture & Shader Slots");
            ImGui::Separator();

            auto renderDropSlot = [&changed](const char* label, char* buffer, size_t bufSize, vesta::asset_payload_type payloadType) {
                ImGui::Text("%s", label);
                std::string btnText = (std::strlen(buffer) > 0) ? buffer : "None (Drag Asset Here)";
                ImGui::Button(btnText.c_str(), ImVec2(-1, 24));
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                        auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);
                        if (payloadType == vesta::asset_payload_type::unspecified || data->type == payloadType) {
                            snprintf(buffer, bufSize, "%s", data->path);
                            changed = true;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            };

            // Custom Shader
            renderDropSlot("Custom Shader:", shader_path, sizeof(shader_path), vesta::asset_payload_type::shader);

            // Albedo / Diffuse
            renderDropSlot("Albedo / Diffuse Texture (.png/.jpg):", diffuse_texture_path, sizeof(diffuse_texture_path), vesta::asset_payload_type::texture);

            // Normal Map
            renderDropSlot("Normal Map Texture:", normal_texture_path, sizeof(normal_texture_path), vesta::asset_payload_type::texture);

            // Metallic Map
            renderDropSlot("Metallic Map Texture (R channel):", metallic_texture_path, sizeof(metallic_texture_path), vesta::asset_payload_type::texture);

            // Roughness Map
            renderDropSlot("Roughness Map Texture (R channel):", roughness_texture_path, sizeof(roughness_texture_path), vesta::asset_payload_type::texture);

            // AO Map
            renderDropSlot("Ambient Occlusion (AO) Texture (R channel):", ao_texture_path, sizeof(ao_texture_path), vesta::asset_payload_type::texture);

            // Emissive Map
            renderDropSlot("Emissive Texture (RGB):", emissive_texture_path, sizeof(emissive_texture_path), vesta::asset_payload_type::texture);

            ImGui::Spacing();

            // ========================================================
            // 4. KAYDETME VE CANLI HOT-RELOAD İŞLEMİ
            // ========================================================
            const bool saveClicked = ImGui::Button("Save Material Configuration", ImVec2(-1, 35));
            if (changed || saveClicked) {

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

                // Shininess fallback hesapla
                float shininessVal = std::pow(1.0f - roughness, 2.0f) * 256.0f;

                std::ostringstream json;
                json << "{\n"
                    << "  \"type\": \"material\",\n"
                    << "  \"name\": \"" << std::filesystem::path(asset_path).stem().string() << "\",\n"
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
                    << "  \"diffuse_texture_guid\": \"" << diffuse_guid_str << "\",\n"
                    << "  \"normal_texture_guid\": \"" << normal_guid_str << "\",\n"
                    << "  \"metallic_texture_guid\": \"" << metallic_guid_str << "\",\n"
                    << "  \"roughness_texture_guid\": \"" << roughness_guid_str << "\",\n"
                    << "  \"ao_texture_guid\": \"" << ao_guid_str << "\",\n"
                    << "  \"emissive_texture_guid\": \"" << emissive_guid_str << "\",\n"
                    << "  \"specular_texture_guid\": \"" << specular_guid_str << "\"\n"
                    << "}";

                // Materyal servisi üzerinden GUID bazlı kayıt ve canlı Hot-Reload işlemi
                aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(asset_path);
                if (aria_sys && !guid.is_nil()) {
                    aria_sys->register_asset_path(guid, asset_path);
                }

                if (mat_service && !guid.is_nil()) {
                    bool ok = mat_service->save_material_changes(guid, json.str());
                    if (ok && saveClicked) {
                        std::cout << "[Vesta MaterialSystem] Basariyla serilestirildi ve kaydedildi: " << asset_path << std::endl;
                    }
                    else if (!ok) {
                        std::cerr << "[Vesta MaterialSystem] Hata: save_material_changes basarisiz oldu: " << asset_path << std::endl;
                    }
                }
                else {
                    std::cerr << "[Vesta MaterialSystem] Hata: material_service gecersiz veya GUID tanimsiz, kaydedilemedi.\n";
                }
            }
        };

        asset_ui_registry::register_asset("material", info);
    }

} // namespace vesta