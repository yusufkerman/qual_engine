#pragma once
#include "../IAssetInspector.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/engine_context.h"
#include <imgui.h>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include "../../Engine/asset_pipeline.h"
#include <glm/glm.hpp> // 🎯 texture_scale için eklendi

struct EditorContext;
struct EngineContext;

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
        int parse_single_int(const std::string& content, const std::string& key, int default_val) {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return default_val;
            size_t colon = content.find(":", pos);
            if (colon == std::string::npos) return default_val;
            size_t end_pos = content.find_first_of(",}\n", colon);
            std::string val_str = content.substr(colon + 1, end_pos - colon - 1);
            try { return std::stoi(val_str); }
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

        // 🎯 ARIA ASSET DATABASE ÜZERİNDEN GÜNCEL DISK YOLUNU ÇÖZEN YARDIMCI METOT
        std::string resolve_path_from_database(const std::string& guid_str, aria::system* aria_sys) {
            if (guid_str == "00000000-0000-0000-0000-000000000000" || guid_str.empty() || !aria_sys) {
                return "";
            }
            auto parsed_guid = uuids::uuid::from_string(guid_str);
            if (parsed_guid) {
                // Aria'nın kendi merkez veritabanına gidip diskteki en taze dosya yolunu kapıyoruz
                return aria_sys->resolve_asset_path(*parsed_guid);
            }
            return "";
        }

    public:
        void on_render_inspector(const std::string& asset_path, EditorContext& editorCtx, EngineContext& engineContext) override {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "📝 Aria Material Inspector");
            ImGui::Text("Config File: %s", std::filesystem::path(asset_path).filename().string().c_str());
            ImGui::Separator();

            // Durum verileri için statik saklama alanları
            static std::string last_loaded_path = "";
            static float base_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            static float specular_strength = 0.5f;
            static float shininess = 32.0f;
            static float normal_strength = 1.0f;
            static float texture_scale[2] = { 1.0f, 1.0f };

            static int render_mode = 0;         // 0: Opaque, 1: Cutout, 2: Transparent
            static float alpha_threshold = 0.1f;

            // Sürükle-Bırak tamponları
            static char shader_path[256] = "";
            static char diffuse_texture_path[256] = "";
            static char normal_texture_path[256] = "";
            static char specular_texture_path[256] = "";

            // 🎯 Referans olarak aldığın engineContext içerisindeki aria_system pointer'ını yakalıyoruz
            aria::system* aria_sys = engineContext.aria_system;

            // CANLI DOSYA OKUMA
            if (last_loaded_path != asset_path) {
                std::string content = read_file_content(asset_path);
                if (!content.empty()) {
                    parse_float_array(content, "base_color", base_color, 4);
                    specular_strength = parse_single_float(content, "specular_strength", 0.5f);
                    shininess = parse_single_float(content, "shininess", 32.0f);
                    normal_strength = parse_single_float(content, "normal_strength", 1.0f);

                    texture_scale[0] = 1.0f; texture_scale[1] = 1.0f;
                    parse_float_array(content, "texture_scale", texture_scale, 2);

                    render_mode = parse_single_int(content, "render_mode", 0);
                    alpha_threshold = parse_single_float(content, "alpha_threshold", 0.1f);

                    // 🎯 1. ADIM: JSON'dan sadece kaydedilmiş ham GUID string'lerini ayıklıyoruz
                    std::string shader_guid = parse_string(content, "shader_guid", "");
                    std::string diffuse_guid = parse_string(content, "diffuse_texture_guid", "");
                    std::string normal_guid = parse_string(content, "normal_texture_guid", "");
                    std::string specular_guid = parse_string(content, "specular_texture_guid", "");

                    // 🎯 2. ADIM: Ayıklanan GUID'leri veritabanına sorup UI isim tamponlarını güncel yollarla dolduruyoruz
                    snprintf(shader_path, sizeof(shader_path), "%s", resolve_path_from_database(shader_guid, aria_sys).c_str());
                    snprintf(diffuse_texture_path, sizeof(diffuse_texture_path), "%s", resolve_path_from_database(diffuse_guid, aria_sys).c_str());
                    snprintf(normal_texture_path, sizeof(normal_texture_path), "%s", resolve_path_from_database(normal_guid, aria_sys).c_str());
                    snprintf(specular_texture_path, sizeof(specular_texture_path), "%s", resolve_path_from_database(specular_guid, aria_sys).c_str());

                    // Geriye dönük uyumluluk/Yedek mekanizma: Eğer database'de yoksa eski düz string mantığını dene
                    if (strlen(shader_path) == 0) {
                        snprintf(shader_path, sizeof(shader_path), "%s", parse_string(content, "shader_path", "Shaders/default_shader").c_str());
                    }
                }
                last_loaded_path = asset_path;
            }

            // YENİ UI ELEMANLARI: Render Ayarları Katmanı
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "🎨 Render Settings");
            ImGui::Separator();

            const char* render_modes[] = { "Opaque (0)", "Cutout / Alpha Test (1)", "Transparent (2)" };
            ImGui::Combo("Render Mode", &render_mode, render_modes, IM_ARRAYSIZE(render_modes));

            // Eğer mod Cutout seçildiyse eşik değeri slider'ını göster
            if (render_mode == 1) {
                ImGui::SliderFloat("Alpha Threshold", &alpha_threshold, 0.0f, 1.0f, "%.2f");
            }
            ImGui::Spacing();

            // Standart Parametreler
            ImGui::ColorEdit4("Base Color", base_color);
            ImGui::DragFloat("Specular Strength", &specular_strength, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Shininess", &shininess, 0.5f, 1.0f, 256.0f);
            ImGui::DragFloat("Normal Strength", &normal_strength, 0.01f, 0.0f, 5.0f);
            ImGui::DragFloat2("Texture Scale (Tiling)", texture_scale, 0.05f, 0.001f, 100.0f);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "🛡️ Aria Resource Bindings");
            ImGui::Separator();

            // ---------------------------------------------------------
            // SHADER DROP ZONE
            // ---------------------------------------------------------
            ImGui::Text("Custom Shader:");
            std::string shader_label = strlen(shader_path) > 0 ? shader_path : "Drag Shader Here (Default Used)";
            ImGui::Button(shader_label.c_str(), ImVec2(-1, 25));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<aria::editor::asset_drag_payload*>(payload->Data);
                    snprintf(shader_path, sizeof(shader_path), "%s", data->path);
                }
                ImGui::EndDragDropTarget();
            }

            // ---------------------------------------------------------
            // DIFFUSE TEXTURE DROP ZONE
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
            // NORMAL TEXTURE DROP ZONE
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
                        << "  \"texture_scale\": [" << texture_scale[0] << ", " << texture_scale[1] << "],\n"
                        << "  \"render_mode\": " << render_mode << ",\n"
                        << "  \"alpha_threshold\": " << alpha_threshold << ",\n"
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