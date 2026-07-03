#pragma once
#include "../IAssetInspector.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include "../../Engine/asset_pipeline.h"

struct EditorContext;

namespace aria::editor {

    class MaterialInspector : public IAssetInspector {
    private:
        // JSON Helper'lar (Yapıyı bozmamak için aynen korundu)
        std::string read_file_content(const std::string& path) {
            std::ifstream f(path);
            if (!f.is_open()) return "";
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }

        void parse_float_array(const std::string& content, const std::string& key, float* out_array, int count) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return;
            size_t start_bracket = content.find("[", pos);
            if (start_bracket == std::string::npos) return;
            size_t end_bracket = content.find("]", start_bracket);
            if (end_bracket == std::string::npos) return;
            std::string array_str = content.substr(start_bracket + 1, end_bracket - start_bracket - 1);
            std::stringstream ss(array_str);
            std::string token; int i = 0;
            while (std::getline(ss, token, ',') && i < count) {
                out_array[i] = std::stof(token); i++;
            }
        }

        float parse_single_float(const std::string& content, const std::string& key, float default_val) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return default_val;
            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return default_val;
            size_t comma = content.find(",", colon);
            std::string val_str = content.substr(colon + 1, comma - colon - 1);
            try { return std::stof(val_str); }
            catch (...) { return default_val; }
        }

        std::string parse_string(const std::string& content, const std::string& key, const std::string& default_val) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return default_val;
            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return default_val;
            size_t first_quote = content.find("\"", colon);
            if (first_quote == std::string::npos) return default_val;
            size_t second_quote = content.find("\"", first_quote + 1);
            if (second_quote == std::string::npos) return default_val;
            return content.substr(first_quote + 1, second_quote - first_quote - 1);
        }

    public:
        void on_render_inspector(const std::string& asset_path, EditorContext& editorCtx) override {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "📝 Aria Material Inspector");
            ImGui::Text("Config File: %s", std::filesystem::path(asset_path).filename().string().c_str());
            ImGui::Separator();

            // Durum verileri için statik saklama alanları
            static std::string last_loaded_path = "";
            static float base_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            static float specular_strength = 0.5f;
            static float shininess = 32.0f;
            static float normal_strength = 1.0f;

            // 🚀 Yeni eklenen shader ve doku yolları (Sürükle-Bırak ile dolacak)
            static char shader_path[256] = "";
            static char diffuse_texture_path[256] = "";
            static char normal_texture_path[256] = "";
            static char specular_texture_path[256] = "";

            // CANLI DOSYA OKUMA
            if (last_loaded_path != asset_path) {
                std::string content = read_file_content(asset_path);
                if (!content.empty()) {
                    parse_float_array(content, "base_color", base_color, 4);
                    specular_strength = parse_single_float(content, "specular_strength", 0.5f);
                    shininess = parse_single_float(content, "shininess", 32.0f);
                    normal_strength = parse_single_float(content, "normal_strength", 1.0f);

                    // String tabanlı Aria kaynak yollarını parse et
                    snprintf(shader_path, sizeof(shader_path), "%s", parse_string(content, "shader_path", "Shaders/default_shader").c_str());
                    snprintf(diffuse_texture_path, sizeof(diffuse_texture_path), "%s", parse_string(content, "diffuse_texture_path", "").c_str());
                    snprintf(normal_texture_path, sizeof(normal_texture_path), "%s", parse_string(content, "normal_texture_path", "").c_str());
                    snprintf(specular_texture_path, sizeof(specular_texture_path), "%s", parse_string(content, "specular_texture_path", "").c_str());
                }
                last_loaded_path = asset_path;
            }

            // Standart Parametreler
            ImGui::ColorEdit4("Base Color", base_color);
            ImGui::DragFloat("Specular Strength", &specular_strength, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Shininess", &shininess, 0.5f, 1.0f, 256.0f);
            ImGui::DragFloat("Normal Strength", &normal_strength, 0.01f, 0.0f, 5.0f);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "🛡️ Aria Resource Bindings");
            ImGui::Separator();

            // ---------------------------------------------------------
            // 🔥 SHADER DROP ZONE
            // ---------------------------------------------------------
            ImGui::Text("Custom Shader:");
            std::string shader_label = strlen(shader_path) > 0 ? shader_path : "Drag Shader Here (Default Used)";
            ImGui::Button(shader_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<aria::editor::asset_drag_payload*>(payload->Data);
                    // Projenizdeki shader uzantılarına göre kontrol edebilirsiniz (.glsl, .vert vb.)
                    snprintf(shader_path, sizeof(shader_path), "%s", data->path);
                }
                ImGui::EndDragDropTarget();
            }

            // ---------------------------------------------------------
            // 🔥 DIFFUSE TEXTURE DROP ZONE
            // ---------------------------------------------------------
            ImGui::Text("Diffuse Texture (.png/.jpg):");
            std::string diffuse_label = strlen(diffuse_texture_path) > 0 ? diffuse_texture_path : "None (Drag Texture)";
            ImGui::Button(diffuse_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<aria::editor::asset_drag_payload*>(payload->Data);
                    if (data->type == aria::editor::asset_payload_type::texture) {
                        snprintf(diffuse_texture_path, sizeof(diffuse_texture_path), "%s", data->path);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // ---------------------------------------------------------
            // 🔥 NORMAL TEXTURE DROP ZONE
            // ---------------------------------------------------------
            ImGui::Text("Normal Map Texture:");
            std::string normal_label = strlen(normal_texture_path) > 0 ? normal_texture_path : "None (Drag Texture)";
            ImGui::Button(normal_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<aria::editor::asset_drag_payload*>(payload->Data);
                    if (data->type == aria::editor::asset_payload_type::texture) {
                        snprintf(normal_texture_path, sizeof(normal_texture_path), "%s", data->path);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::Spacing();

            // SAVE BUTONU
            if (ImGui::Button("Save Material Configuration", ImVec2(-1, 35))) {
                std::ofstream file(asset_path);
                if (file.is_open()) {

                    // Sürüklenen yollar boş değilse meta dosyalarından GUID'lerini çekiyoruz, 
                    // Boşsa geçersiz/boş bir GUID string'i ("00000000-...") bırakıyoruz.
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

                    file << "{\n"
                        << "  \"type\": \"material\",\n"
                        << "  \"name\": \"" << std::filesystem::path(asset_path).stem().string() << "\",\n"
                        << "  \"base_color\": [" << base_color[0] << ", " << base_color[1] << ", " << base_color[2] << ", " << base_color[3] << "],\n"
                        << "  \"specular_strength\": " << specular_strength << ",\n"
                        << "  \"shininess\": " << shininess << ",\n"
                        << "  \"normal_strength\": " << normal_strength << ",\n"
                        << "  \"shader_guid\": \"" << shader_guid_str << "\",\n"
                        << "  \"diffuse_texture_guid\": \"" << diffuse_guid_str << "\",\n"
                        << "  \"normal_texture_guid\": \"" << normal_guid_str << "\",\n"
                        << "  \"specular_texture_guid\": \"" << specular_guid_str << "\"\n"
                        << "}";

                    file.close();
                    std::cout << "[Material System] Serialized & Saved via GUIDs to: " << asset_path << std::endl;
                }
            }
        }
    };

} // namespace aria::editor