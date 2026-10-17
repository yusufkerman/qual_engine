/**
 * @file rendering_panel.cpp
 * @brief Tüm grafik arka uç parametrelerinin arayüz ve OpenGL durum senkronizasyon uygulaması.
 */

#include "rendering_panel.h"
#include "../../../Engine/engine_context.h"
#include "../../../Importers/cubemap_loader.h"
#include "../../../Project/project.h"
#include "../../Externals/nfd/nfd.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <iostream>

namespace vesta {

    rendering_panel::rendering_panel(EngineContext& engineCtx)
        : m_engine_ctx(engineCtx)
    {
    }

    void rendering_panel::render()
    {
        if (!m_is_open)
            return;

        ImGui::Begin(m_title.c_str(), &m_is_open);

        ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Graphics Pipeline & Backend Rendering Settings");
        ImGui::TextDisabled("Configure engine rendering features, optimizations, and visual parameters.");

        // Proje durumu ve ayarları kaydetme butonu
        const bool hasProj = project::project_manager::instance().has_active_project();
        if (hasProj)
        {
            ImGui::Text("Project: %s", project::project_manager::instance().get_project_name().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Save to Project"))
            {
                project::project_manager::instance().save_project(m_engine_ctx);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Saves current rendering settings directly into project.json");
            }
        }
        else
        {
            ImGui::TextDisabled("No active project. (Use File -> New Project / Open Project)");
        }

        ImGui::Separator();

        auto& settings = m_engine_ctx.renderSettings;

        // ====================================================================
        // 1. EKRAN, DONANIM VE RASTERİZER (HARDWARE & DISPLAY)
        // ====================================================================
        if (ImGui::CollapsingHeader("Display & Hardware", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // VSync Ayarı
            bool vsyncVal = settings.vsync;
            if (ImGui::Checkbox("Vertical Sync (VSync)", &vsyncVal))
            {
                settings.vsync = vsyncVal;
                glfwSwapInterval(settings.vsync ? 1 : 0);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Synchronizes frame rate to monitor refresh rate (Caps at monitor Hz vs. Uncapped FPS).");
            }

            ImGui::SameLine();
            ImGui::TextDisabled("(Current: %s)", settings.vsync ? "Enabled (VSync On)" : "Disabled (Uncapped)");

            // Wireframe Modu
            bool wireframeVal = settings.wireframe;
            if (ImGui::Checkbox("Wireframe Mode (Tel Kafes)", &wireframeVal))
            {
                settings.wireframe = wireframeVal;
                glPolygonMode(GL_FRONT_AND_BACK, settings.wireframe ? GL_LINE : GL_FILL);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Renders all geometry in polygon wireframe outline mode.");
            }

            ImGui::SameLine();

            // Backface Culling
            bool cullVal = settings.backface_culling;
            if (ImGui::Checkbox("Backface Culling (Arka Yüz Kırpma)", &cullVal))
            {
                settings.backface_culling = cullVal;
                if (settings.backface_culling)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
                else
                {
                    glDisable(GL_CULL_FACE);
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Discards back-facing triangles to boost performance on closed solid meshes.");
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 2. GÖRÜŞ ALANI KIRPMA (FRUSTUM CULLING & OPTIMIZATION)
        // ====================================================================
        if (ImGui::CollapsingHeader("Frustum Culling & Optimization", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enable Frustum Culling", &settings.enable_frustum_culling);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Discards objects outside the active camera view frustum to save GPU draw calls and vertex workload.");
            }

            if (settings.enable_frustum_culling)
            {
                ImGui::SliderFloat("Cull Radius Bias", &settings.cull_bias, 0.0f, 0.20f, "%.3f m");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Safety margin added to object bounding sphere radius to prevent edge popping.");
                }
            }

            // Anlık Frustum Culling Telemetrisi
            const auto& sceneStats = m_engine_ctx.renderSystem.scene_stats();
            const uint32_t totalSubmeshes = sceneStats.visible_submeshes + sceneStats.culled_submeshes;
            const float cullPercent = totalSubmeshes > 0
                ? (static_cast<float>(sceneStats.culled_submeshes) / static_cast<float>(totalSubmeshes)) * 100.0f
                : 0.0f;

            ImGui::Text("Frustum Telemetry: %u visible | %u culled (Total: %u submeshes, %.1f%% culled)",
                sceneStats.visible_submeshes, sceneStats.culled_submeshes, totalSubmeshes, cullPercent);
        }

        ImGui::Spacing();

        // ====================================================================
        // 3. GÖLGE HARİTALAMA (DIRECTIONAL SHADOW MAPPING)
        // ====================================================================
        if (ImGui::CollapsingHeader("Shadow Mapping (Yönlü Işık Gölgeleri)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enable Directional Shadows", &settings.enable_shadows);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Toggles the 32-bit depth buffer directional light shadow pass.");
            }

            if (settings.enable_shadows)
            {
                // Gölge Çözünürlüğü Seçici
                const char* resOptions[] = { "512 x 512 (Low)", "1024 x 1024 (Medium)", "2048 x 2048 (High - Default)", "4096 x 4096 (Ultra)" };
                int currentResIdx = 2;
                if (settings.shadow_map_resolution == 512) currentResIdx = 0;
                else if (settings.shadow_map_resolution == 1024) currentResIdx = 1;
                else if (settings.shadow_map_resolution == 2048) currentResIdx = 2;
                else if (settings.shadow_map_resolution == 4096) currentResIdx = 3;

                ImGui::SetNextItemWidth(220.0f);
                if (ImGui::Combo("Shadow Resolution", &currentResIdx, resOptions, IM_ARRAYSIZE(resOptions)))
                {
                    int newRes = 2048;
                    if (currentResIdx == 0) newRes = 512;
                    else if (currentResIdx == 1) newRes = 1024;
                    else if (currentResIdx == 2) newRes = 2048;
                    else if (currentResIdx == 3) newRes = 4096;

                    if (newRes != settings.shadow_map_resolution)
                    {
                        settings.shadow_map_resolution = newRes;
                        m_engine_ctx.shadowFBO.init(newRes, newRes);
                        std::cout << "[RenderingPanel] Golge FBO cozunurlugu guncellendi: " << newRes << "x" << newRes << "\n";
                    }
                }

                ImGui::DragFloat("Ortho Projection Size", &settings.shadow_ortho_size, 1.0f, 20.0f, 600.0f, "%.1f m");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Orthographic bounding extent of the directional shadow camera (Half-size).");
                }

                ImGui::DragFloat("Shadow Follow Distance", &settings.shadow_distance, 1.0f, 10.0f, 500.0f, "%.1f m");
                ImGui::DragFloat("Shadow Camera Height", &settings.shadow_camera_height, 1.0f, 10.0f, 500.0f, "%.1f m");
                ImGui::DragFloat("Shadow Near Plane", &settings.shadow_near_plane, 0.05f, 0.01f, 20.0f, "%.2f m");
                ImGui::DragFloat("Shadow Far Plane", &settings.shadow_far_plane, 5.0f, 50.0f, 2000.0f, "%.1f m");

                // Gölge Derinlik Dokusu Önizlemesi
                ImGui::Checkbox("Show Shadow Map Depth Preview", &settings.show_shadow_map_debug);
                if (settings.show_shadow_map_debug && m_engine_ctx.shadowFBO.get_depth_texture() != 0)
                {
                    ImGui::Text("Shadow Texture (ID: %u, Res: %ux%u):",
                        m_engine_ctx.shadowFBO.get_depth_texture(),
                        m_engine_ctx.shadowFBO.get_width(),
                        m_engine_ctx.shadowFBO.get_height());
                    ImGui::Image(
                        (ImTextureID)(intptr_t)m_engine_ctx.shadowFBO.get_depth_texture(),
                        ImVec2(180.0f, 180.0f),
                        ImVec2(0.0f, 1.0f),
                        ImVec2(1.0f, 0.0f)
                    );
                }
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 4. GÖKYÜZÜ KUTUSU VE ARKA PLAN (SKYBOX & ENVIRONMENT)
        // ====================================================================
        if (ImGui::CollapsingHeader("Skybox & Environment", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enable Skybox Cubemap", &settings.enable_skybox);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Renders the 360-degree skybox cubemap behind all opaque and transparent scene geometry.");
            }

            ImGui::TextDisabled("Skybox Atlas Path: %s", settings.skybox_path.c_str());

            if (ImGui::Button("Change Skybox Atlas..."))
            {
                nfdu8char_t* outPath = nullptr;
                nfdu8filteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga" } };

                if (NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr) == NFD_OKAY)
                {
                    if (outPath != nullptr)
                    {
                        const unsigned int newTex = cubemap_loader::load_atlas(outPath);
                        if (newTex != 0)
                        {
                            if (m_engine_ctx.skyboxTexture != 0)
                            {
                                glDeleteTextures(1, &m_engine_ctx.skyboxTexture);
                            }
                            m_engine_ctx.skyboxTexture = newTex;
                            settings.skybox_path = outPath;
                            std::cout << "[RenderingPanel] Yeni skybox atlas basariyla yuklendi: " << outPath << "\n";
                        }
                        NFD_FreePathU8(outPath);
                    }
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Load a new 4x3 horizontal cross cubemap atlas image (PNG, JPG, TGA).");
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Fallback Clear Colors (When Skybox is Off)");
            ImGui::ColorEdit3("Scene View Background", &settings.clear_color_scene.x, ImGuiColorEditFlags_Float);
            ImGui::ColorEdit3("Game View Background", &settings.clear_color_game.x, ImGuiColorEditFlags_Float);
        }

        ImGui::Spacing();

        // ====================================================================
        // 5. ÜSTEL SİS (EXPONENTIAL FOG)
        // ====================================================================
        if (ImGui::CollapsingHeader("Atmosphere & Exponential Fog", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& fog = m_engine_ctx.fog;

            ImGui::Checkbox("Enable Fog (Game)", &fog.enabled);
            ImGui::SameLine();
            ImGui::Checkbox("Show Fog in Scene View", &fog.show_in_scene);

            ImGui::ColorEdit3("Fog Color", &fog.color.x, ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("Fog Density", &fog.density, 0.0001f, 0.0400f, "%.4f");

            ImGui::TextDisabled("Atmospheric Presets:");
            if (ImGui::SmallButton("Clear")) { fog.enabled = true; fog.density = 0.003f; fog.color = glm::vec3(0.70f, 0.80f, 0.90f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Morning Mist")) { fog.enabled = true; fog.density = 0.010f; fog.color = glm::vec3(0.75f, 0.82f, 0.92f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Dense Fog")) { fog.enabled = true; fog.density = 0.025f; fog.color = glm::vec3(0.60f, 0.65f, 0.70f); }
            ImGui::SameLine();
            if (ImGui::SmallButton("Sunset Dusk")) { fog.enabled = true; fog.density = 0.009f; fog.color = glm::vec3(0.85f, 0.45f, 0.30f); }
        }

        ImGui::Spacing();

        // ====================================================================
        // 6. MOTOR GRAFİK YETENEKLERİ ÖZETİ (CAPABILITIES SUMMARY)
        // ====================================================================
        if (ImGui::CollapsingHeader("Active Graphics Pipeline Capabilities"))
        {
            ImGui::BulletText("API: OpenGL 3.3 Core Profile (Forward Pipeline)");
            ImGui::BulletText("Shading Model: Blinn-Phong + Normal Mapping + Specular Shininess");
            ImGui::BulletText("Skinning: GPU Linear Blend Skinning (128 bones per skeleton via UBO binding #1)");
            ImGui::BulletText("Lighting: Multi-Light UBO (Up to 64 active Directional, Point, and Spot lights)");
            ImGui::BulletText("Shadows: 32-bit Directional Light Shadow Mapping (Binding #2)");
            ImGui::BulletText("Atmosphere: Exponential Distance Fog (GLSL Shader Calculation)");
            ImGui::BulletText("Culling: Exact 6-Plane Camera Frustum Bounding Sphere Culling");
            ImGui::BulletText("Batching: Aria Asset Pipeline GPU Shader-State Sorting");
        }

        ImGui::End();
    }

} // namespace vesta

