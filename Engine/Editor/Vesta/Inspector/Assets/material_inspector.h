/**
 * @file material_asset_ui.h
 * @brief Material asset'i (.json) için editör Inspector arayüzü.
 *        component_ui_registry pattern'iyle birebir aynı mantık: SAF VESTA,
 *        dış bağımlılıklar (aria::system*, material_service*) register anında
 *        closure içine yakalanır, render_fn hiçbir context taşımaz.
 */
#pragma once

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

namespace vesta {

    namespace detail {

        inline std::string mat_read_file(const std::string& path) {
            std::ifstream f(path);
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
            if (eb == std::string::npos) return;
            std::stringstream ss(content.substr(sb + 1, eb - sb - 1));
            std::string tok; int i = 0;
            while (std::getline(ss, tok, ',') && i < count) { out[i] = std::stof(tok); i++; }
        }

        inline float mat_parse_float(const std::string& content, const std::string& key, float def) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return def;
            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return def;
            size_t comma = content.find_first_of(",}\n", colon);
            try { return std::stof(content.substr(colon + 1, comma - colon - 1)); }
            catch (...) { return def; }
        }

        inline int mat_parse_int(const std::string& content, const std::string& key, int def) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return def;
            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return def;
            size_t end_pos = content.find_first_of(",}\n", colon);
            try { return std::stoi(content.substr(colon + 1, end_pos - colon - 1)); }
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
            if (guid_str.empty() || guid_str == "00000000-0000-0000-0000-000000000000" || !aria_sys) return "";
            auto parsed = uuids::uuid::from_string(guid_str);
            if (parsed) return aria_sys->resolve_asset_path(*parsed);
            return "";
        }

    } // namespace detail

    /**
     * @brief Material inspector'ı asset_ui_registry'e kaydeder.
     *        aria_sys ve mat_service init anında closure'a yakalanır.
     */
    inline void register_material_asset_ui_system(aria::system* aria_sys, material_service* mat_service) {

        AssetUIInfo info;
        info.name = "Material";

        info.can_inspect_fn = [](const std::string& asset_path) -> bool {
            std::string ext = std::filesystem::path(asset_path).extension().string();
            return ext == ".json" || ext == ".amat";
            };

        info.render_fn = [aria_sys, mat_service](const std::string& asset_path) {

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

            if (last_loaded_path != asset_path) {
                std::string content = detail::mat_read_file(asset_path);
                if (!content.empty()) {
                    detail::mat_parse_float_array(content, "base_color", base_color, 4);
                    specular_strength = detail::mat_parse_float(content, "specular_strength", 0.5f);
                    shininess = detail::mat_parse_float(content, "shininess", 32.0f);
                    normal_strength = detail::mat_parse_float(content, "normal_strength", 1.0f);

                    texture_scale[0] = 1.0f; texture_scale[1] = 1.0f;
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

                    if (strlen(shader_path) == 0) {
                        snprintf(shader_path, sizeof(shader_path), "%s", detail::mat_parse_string(content, "shader_path", "Shaders/default_shader").c_str());
                    }
                }
                last_loaded_path = asset_path;
            }

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

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Aria Resource Bindings");
            ImGui::Separator();

            ImGui::Text("Custom Shader:");
            std::string shader_label = strlen(shader_path) > 0 ? shader_path : "Drag Shader Here (Default Used)";
            ImGui::Button(shader_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<vesta::asset_drag_payload*>(payload->Data);
                    snprintf(shader_path, sizeof(shader_path), "%s", data->path);
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::Text("Diffuse Texture (.png/.jpg):");
            std::string diffuse_label = strlen(diffuse_texture_path) > 0 ? diffuse_texture_path : "None (Drag Texture)";
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

            ImGui::Text("Normal Map Texture:");
            std::string normal_label = strlen(normal_texture_path) > 0 ? normal_texture_path : "None (Drag Texture)";
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

            if (ImGui::Button("Save Material Configuration", ImVec2(-1, 35))) {

                std::string shader_guid_str = (strlen(shader_path) > 0)
                    ? uuids::to_string(asset_pipeline::get_or_create_meta_guid(shader_path))
                    : "00000000-0000-0000-0000-000000000000";
                std::string diffuse_guid_str = (strlen(diffuse_texture_path) > 0)
                    ? uuids::to_string(asset_pipeline::get_or_create_meta_guid(diffuse_texture_path))
                    : "00000000-0000-0000-0000-000000000000";
                std::string normal_guid_str = (strlen(normal_texture_path) > 0)
                    ? uuids::to_string(asset_pipeline::get_or_create_meta_guid(normal_texture_path))
                    : "00000000-0000-0000-0000-000000000000";
                std::string specular_guid_str = (strlen(specular_texture_path) > 0)
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

                // 🎯 Artık raw ofstream yok — material_service üzerinden GUID ile kaydediliyor.
                // Bu, mark_scene_dirty() tetiklenmesini de garanti eder.
                aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(asset_path);
                if (mat_service && !guid.is_nil()) {
                    bool ok = mat_service->save_material_changes(guid, json.str());
                    if (ok) {
                        std::cout << "[Material System] Serialized & Saved via material_service: " << asset_path << std::endl;
                    }
                    else {
                        std::cerr << "[Material System] save_material_changes basarisiz: " << asset_path << std::endl;
                    }
                }
                else {
                    std::cerr << "[Material System] material_service yok veya guid nil, kaydedilemedi.\n";
                }
            }
            };

        asset_ui_registry::register_asset("material", info);
    }

} // namespace vesta