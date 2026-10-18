#pragma once

/**
 * @file texture_inspector.h
 * @brief Doku/Görsel varlıkları (.png, .jpg, .jpeg, .tga) için Inspector paneli arayüz kaydını,
 * önizleme (preview), filtreleme modu, sarma (wrap) modu ve hot-reload işlemlerini barındırır.
 */

#include "../../Services/asset_ui_registry.h"
#include "../../Services/asset_entry.h"
#include "../../../Engine/Assets/aria.h"
#include "../../../Engine/asset_pipeline.h"

#include <glad/glad.h>
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace vesta {

    /**
     * @brief Doku (Texture) varlık türü için editör Inspector arayüz sistemini kaydeder.
     * @param aria_sys ARIA varlık yönetim sistemi göstericisi.
     */
    inline void register_texture_asset_ui_system(aria::system* aria_sys) {
        if (!aria_sys) return;

        AssetUIInfo info;
        info.name = "Texture Asset";
        info.render_fn = [aria_sys](const std::string& asset_path) {
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

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Texture Importer");
            ImGui::Separator();

            ImGui::Text("File: %s", filename.c_str());
            ImGui::TextDisabled("Path: %s", asset_path.c_str());
            ImGui::Text("Size: %.2f KB", static_cast<float>(fileSize) / 1024.0f);

            // ARIA GUID ve Texture handle'ını çöz
            aria::asset_guid guid = asset_pipeline::get_or_create_meta_guid(asset_path);
            aria::handle<aria::texture_asset> texHandle = aria_sys->import_texture(guid, asset_path);
            aria::gpu_texture_id gpuTexId = aria_sys->get_texture_id(texHandle);

            // Doku detayları
            int texW = 0, texH = 0;
            if (gpuTexId != 0) {
                glBindTexture(GL_TEXTURE_2D, gpuTexId);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            if (texW > 0 && texH > 0) {
                ImGui::Text("Dimensions: %d x %d px", texW, texH);
            }

            ImGui::Spacing();
            ImGui::Separator();

            // ÖNİZLEME (PREVIEW)
            if (gpuTexId != 0) {
                ImGui::Text("Preview:");
                const float availWidth = ImGui::GetContentRegionAvail().x;
                float previewW = availWidth > 20.0f ? availWidth : 200.0f;
                float previewH = previewW;
                if (texW > 0 && texH > 0) {
                    float aspect = static_cast<float>(texW) / static_cast<float>(texH);
                    previewH = previewW / aspect;
                }
                if (previewH > 220.0f) {
                    float scale = 220.0f / previewH;
                    previewH = 220.0f;
                    previewW *= scale;
                }

                // Dama tahtası / arka plan kutusu
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1(p0.x + previewW, p0.y + previewH);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(p0, p1, IM_COL32(28, 30, 36, 255), 4.0f);
                dl->AddRect(p0, p1, IM_COL32(60, 65, 80, 200), 4.0f);

                ImGui::Image((ImTextureID)(intptr_t)gpuTexId, ImVec2(previewW, previewH), ImVec2(0, 0), ImVec2(1, 1));
            }

            ImGui::Spacing();
            ImGui::Separator();

            // İÇE AKTARMA AYARLARI (IMPORT SETTINGS)
            ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.2f, 1.0f), "Import Settings");

            static int filterMode = 1; // 0: Nearest, 1: Bilinear, 2: Trilinear
            static int wrapMode = 0;   // 0: Repeat, 1: Clamp, 2: Mirrored Repeat
            static bool generateMipmaps = true;

            const char* filterOptions[] = { "Point (Nearest)", "Bilinear (Linear)", "Trilinear (Mipmaps)" };
            ImGui::Combo("Filter Mode", &filterMode, filterOptions, 3);

            const char* wrapOptions[] = { "Repeat", "Clamp to Edge", "Mirrored Repeat" };
            ImGui::Combo("Wrap Mode", &wrapMode, wrapOptions, 3);

            ImGui::Checkbox("Generate Mipmaps", &generateMipmaps);

            ImGui::Spacing();

            if (ImGui::Button("Apply & Reimport", ImVec2(-1.0f, 32.0f))) {
                if (gpuTexId != 0) {
                    glBindTexture(GL_TEXTURE_2D, gpuTexId);

                    GLint minFilter = GL_LINEAR;
                    GLint magFilter = GL_LINEAR;
                    if (filterMode == 0) {
                        minFilter = GL_NEAREST;
                        magFilter = GL_NEAREST;
                    } else if (filterMode == 1) {
                        minFilter = generateMipmaps ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;
                        magFilter = GL_LINEAR;
                    } else {
                        minFilter = GL_LINEAR_MIPMAP_LINEAR;
                        magFilter = GL_LINEAR;
                    }

                    GLint glWrap = GL_REPEAT;
                    if (wrapMode == 1) glWrap = GL_CLAMP_TO_EDGE;
                    else if (wrapMode == 2) glWrap = GL_MIRRORED_REPEAT;

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrap);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrap);

                    if (generateMipmaps) {
                        glGenerateMipmap(GL_TEXTURE_2D);
                    }

                    glBindTexture(GL_TEXTURE_2D, 0);
                    std::cout << "[Vesta TextureSystem] Doku parametreleri guncellendi: " << filename << std::endl;
                }
            }
        };

        asset_ui_registry::register_asset("texture", info);
    }

} // namespace vesta
