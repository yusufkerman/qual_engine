#pragma once

/**
 * @file material_asset_ui.h
 * @brief Materyal varlıkları (.json, .amat) için Inspector paneli arayüz kaydını ve çizim mantığını barındırır.
 * @note Dış servis bağımlılıkları (Aria ve MaterialService) kayıt anında closure içerisine yakalanır.
 */

#include "../../Services/asset_ui_registry.h"
#include "../../Services/material_service.h"
#include "../../../Engine/asset_pipeline.h"
#include "../../../Externals/uuid/uuid.h"
#include "../../Services/asset_entry.h"

#include <imgui.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <cstring>

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
            if (guid_str.empty() || guid_str == "00000000-0000-0000-0000-000000000000" || !aria_sys) return "";
            auto parsed = uuids::uuid::from_string(guid_str);
            if (parsed) return aria_sys->resolve_asset_path(*parsed);
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
            static float specular_strength = 0.5f;
            static float shininess = 32.0f;
            static float normal_strength = 1.0f;
            static float texture_scale[2] = { 1.0f, 1.0f };
            static int render_mode = 0;
            static float alpha_threshold = 0.1f;

            static char shader_path[256] = "";
            static char diffuse_texture_path[256] = "";
            static char normal_texture_path[256] = "";
            static char specular_texture_path[256] = "";

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Material Inspector");
            ImGui::Text("Config File: %s", std::filesystem::path(asset_path).filename().string().c_str());
            ImGui::Separator();

            // Dosya değiştiğinde diskteki güncel JSON verilerini tamponlara oku
            if (last_loaded_path != asset_path) {
                std::string content = detail::mat_read_file(asset_path);
                if (!content.empty()) {
                    detail::mat_parse_float_array(content, "base_color", base_color, 4);
                    specular_strength = detail::mat_parse_float(content, "specular_strength", 0.5f);
                    shininess = detail::mat_parse_float(content, "shininess", 32.0f);
                    normal_strength = detail::mat_parse_float(content, "normal_strength", 1.0f);

                    texture_scale[0] = 1.0f; 
                    texture_scale[1] = 1.0f;
                    detail::mat_parse_float_array(content, "texture_scale", texture_scale, 2);

                    render_mode = detail::mat_parse_int(content, "render_mode", 0);
                    alpha_threshold = detail::mat_parse_float(content, "alpha_threshold", 0.1f);

                    std::string shader_guid = detail::mat_parse_string(content, "shader_guid", "");
                    std::string diffuse_guid = detail::mat_parse_string(content, "diffuse_texture_guid", "");
                    std::string normal_guid = detail::mat_parse_string(content, "normal_texture_guid", "");
                    std::string specular_guid = detail::mat_parse_string(content, "specular_texture_guid", "");

                    snprintf(shader_path, sizeof(shader_path), "%s", detail::mat_resolve_guid_path(shader_guid, aria_sys).c_str());
                    snprintf(diffuse_texture_path, sizeof(diffuse_texture_path), "%s", detail::mat_resolve_guid_path(diffuse_guid, aria_sys).c_str());
                    snprintf(normal_texture_path, sizeof(normal_texture_path), "%s", detail::mat_resolve_guid_path(normal_guid, aria_sys).c_str());
                    snprintf(specular_texture_path, sizeof(specular_texture_path), "%s", detail::mat_resolve_guid_path(specular_guid, aria_sys).c_str());

                    if (std::strlen(shader_path) == 0) {
                        snprintf(shader_path, sizeof(shader_path), "%s", detail::mat_parse_string(content, "shader_path", "Shaders/default_shader").c_str());
                    }
                }
                last_loaded_path = asset_path;
            }

            // ========================================================
            // RENDER AYARLARI
            // ========================================================
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Render Settings");
            ImGui::Separator();

            const char* render_modes[] = { "Opaque (0)", "Cutout / Alpha Test (1)", "Transparent (2)" };
            ImGui::Combo("Render Mode", &render_mode, render_modes, IM_ARRAYSIZE(render_modes));
            if (render_mode == 1) {
                ImGui::SliderFloat("Alpha Threshold", &alpha_threshold, 0.0f, 1.0f, "%.2f");
            }
            ImGui::Spacing();

            ImGui::ColorEdit4("Base Color", base_color);
            ImGui::DragFloat("Specular Strength", &specular_strength, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Shininess", &shininess, 0.5f, 1.0f, 256.0f);
            ImGui::DragFloat("Normal Strength", &normal_strength, 0.01f, 0.0f, 5.0f);
            ImGui::DragFloat2("Texture Scale (Tiling)", texture_scale, 0.05f, 0.001f, 100.0f);

            // ========================================================
            // KAYNAK (ASSET/RESOURCE) BAĞLANTILARI
            // ========================================================
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Aria Resource Bindings");
            ImGui::Separator();

            // Shader Seçimi
            ImGui::Text("Custom Shader:");
            std::string shader_label = std::strlen(shader_path) > 0 ? shader_path : "Drag Shader Here (Default Used)";
            ImGui::Button(shader_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);
                    snprintf(shader_path, sizeof(shader_path), "%s", data->path);
                }
                ImGui::EndDragDropTarget();
            }

            // Diffuse Doku Seçimi
            ImGui::Text("Diffuse Texture (.png/.jpg):");
            std::string diffuse_label = std::strlen(diffuse_texture_path) > 0 ? diffuse_texture_path : "None (Drag Texture)";
            ImGui::Button(diffuse_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);
                    if (data->type == vesta::asset_payload_type::texture) {
                        snprintf(diffuse_texture_path, sizeof(diffuse_texture_path), "%s", data->path);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Normal Map Doku Seçimi
            ImGui::Text("Normal Map Texture:");
            std::string normal_label = std::strlen(normal_texture_path) > 0 ? normal_texture_path : "None (Drag Texture)";
            ImGui::Button(normal_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);
                    if (data->type == vesta::asset_payload_type::texture) {
                        snprintf(normal_texture_path, sizeof(normal_texture_path), "%s", data->path);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::Spacing();

            // ========================================================
            // KAYDETME İŞLEMİ
            // ========================================================
            if (ImGui::Button("Save Material Configuration", ImVec2(-1, 35))) {

                std::string shader_guid_str = (std::strlen(shader_path) > 0)
                    ? uuids::to_string(asset_pipeline::get_or_create_meta_guid(shader_path))
                    : "00000000-0000-0000-0000-000000000000";
                std::string diffuse_guid_str = (std::strlen(diffuse_texture_path) > 0)
                    ? uuids::to_string(asset_pipeline::get_or_create_meta_guid(diffuse_texture_path))
                    : "00000000-0000-0000-0000-000000000000";
                std::string normal_guid_str = (std::strlen(normal_texture_path) > 0)
                    ? uuids::to_string(asset_pipeline::get_or_create_meta_guid(normal_texture_path))
                    : "00000000-0000-0000-0000-000000000000";
                std::string specular_guid_str = (std::strlen(specular_texture_path) > 0)
                    ? uuids::to_string(asset_pipeline::get_or_create_meta_guid(specular_texture_path))
                    : "00000000-0000-0000-0000-000000000000";

                std::ostringstream json;
                json << "{\n"
                    << "  \"type\": \"material\",\n"
                    << "  \"name\": \"" << std::filesystem::path(asset_path).stem().string() << "\",\n"
                    << "  \"base_color\": [" << base_color[0] << ", " << base_color[1] << ", " << base_color[2] << ", " << base_color[3] << "],\n"
                    << "  \"specular_strength\": " << specular_strength << ",\n"
                    << "  \"shininess\": " << shininess << ",\n"
                    << "  \"normal_strength\": " << normal_strength << ",\n"
                    << "  \"texture_scale\": [" << texture_scale[0] << ", " << texture_scale[1] << "],\n"
                    << "  \"render_mode\": " << render_mode << ",\n"
                    << "  \"alpha_threshold\": " << alpha_threshold << ",\n"
                    << "  \"shader_guid\": \"" << shader_guid_str << "\",\n"
                    << "  \"diffuse_texture_guid\": \"" << diffuse_guid_str << "\",\n"
                    << "  \"normal_texture_guid\": \"" << normal_guid_str << "\",\n"
                    << "  \"specular_texture_guid\": \"" << specular_guid_str << "\"\n"
                    << "}";

                // Materyal servisi üzerinden GUID bazlı kayıt işlemi
                aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(asset_path);
                if (mat_service && !guid.is_nil()) {
                    bool ok = mat_service->save_material_changes(guid, json.str());
                    if (ok) {
                        std::cout << "[Vesta MaterialSystem] Basariyla serilestirildi ve kaydedildi: " << asset_path << std::endl;
                    }
                    else {
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