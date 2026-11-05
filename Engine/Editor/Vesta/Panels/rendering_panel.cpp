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
        // 3.5. HACİMSEL IŞIKLANDIRMA (VOLUMETRIC LIGHTING / GOD RAYS)
        // ====================================================================
        if (ImGui::CollapsingHeader("Volumetric Lighting (Işık Süzmeleri / God Rays)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Enable Volumetric Lighting", &settings.enable_volumetric_lighting))
            {
                m_engine_ctx.renderSettings.enable_volumetric_lighting = settings.enable_volumetric_lighting;
                project::project_manager::instance().save_project(m_engine_ctx);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Yönlü ışıktan ve gölgelerden gerçek zamanlı hacimsel ışık süzmeleri (God Rays / Crepuscular Rays) üretir.");
            }

            if (settings.enable_volumetric_lighting)
            {
                if (ImGui::SliderFloat("Intensity", &settings.volumetric_intensity, 0.0f, 5.0f, "%.2f"))
                {
                    m_engine_ctx.renderSettings.volumetric_intensity = settings.volumetric_intensity;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hacimsel ışığın parlaklık ve görünürlük çarpanı.");
                }

                if (ImGui::SliderInt("Sample Steps", &settings.volumetric_steps, 8, 64))
                {
                    m_engine_ctx.renderSettings.volumetric_steps = settings.volumetric_steps;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Işın yürütme (Raymarching) örneklem adım sayısı (Daha yüksek = daha pürüzsüz).");
                }

                if (ImGui::SliderFloat("Scattering (g)", &settings.volumetric_scattering, 0.0f, 0.95f, "%.2f"))
                {
                    m_engine_ctx.renderSettings.volumetric_scattering = settings.volumetric_scattering;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Henyey-Greenstein asimetri saçılma faktörü g (Işığa doğru bakıldığında parlama gücü).");
                }

                if (ImGui::SliderFloat("Density", &settings.volumetric_density, 0.001f, 0.200f, "%.4f"))
                {
                    m_engine_ctx.renderSettings.volumetric_density = settings.volumetric_density;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Atmosferdeki sis ve partikül ortam yoğunluğu.");
                }

                if (ImGui::SliderFloat("Max Distance", &settings.volumetric_max_distance, 10.0f, 300.0f, "%.1f m"))
                {
                    m_engine_ctx.renderSettings.volumetric_max_distance = settings.volumetric_max_distance;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Işın yürütmenin hesaplanacağı azami görüş mesafesi.");
                }
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 3.6. ORTAM GÖLGELEMESİ (AMBIENT OCCLUSION / SSAO & GTAO)
        // ====================================================================
        if (ImGui::CollapsingHeader("Ambient Occlusion (SSAO / GTAO)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Enable Ambient Occlusion", &settings.enable_ssao))
            {
                m_engine_ctx.renderSettings.enable_ssao = settings.enable_ssao;
                project::project_manager::instance().save_project(m_engine_ctx);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Köşe, girinti, tünel ve temas yüzeylerinde gerçek zamanlı ekran uzayı ortam gölgelenmesi hesaplar.");
            }

            if (settings.enable_ssao)
            {
                const char* aoMethodNames[] = { "SSAO (Klasik)", "GTAO (Ground Truth / PBR)" };
                int currentAOMethod = settings.ao_method;
                if (ImGui::Combo("AO Metodu", &currentAOMethod, aoMethodNames, IM_ARRAYSIZE(aoMethodNames)))
                {
                    settings.ao_method = currentAOMethod;
                    m_engine_ctx.renderSettings.ao_method = currentAOMethod;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("SSAO: Hızlı yarım küre örneklemesi.\nGTAO: Fiziksel tabanlı, tünelleri ve tavanları doğal karartan analitik ufuk integrali (Standart PBR).");
                }

                if (settings.ao_method == 1) // GTAO
                {
                    if (ImGui::SliderFloat("Radius", &settings.ssao_radius, 0.1f, 8.0f, "%.2f m"))
                    {
                        m_engine_ctx.renderSettings.ssao_radius = settings.ssao_radius;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Mekansal etki yarıçapı. Tünel ve salonların doğal kararması için 2.0m - 5.0m arası önerilir.");
                    }

                    if (ImGui::SliderFloat("Intensity", &settings.ssao_intensity, 0.1f, 4.0f, "%.2f"))
                    {
                        m_engine_ctx.renderSettings.ssao_intensity = settings.ssao_intensity;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Gölge koyuluk ve kontrast üssü.");
                    }

                    if (ImGui::SliderInt("Directions (Dilim)", &settings.gtao_directions, 1, 6))
                    {
                        m_engine_ctx.renderSettings.gtao_directions = settings.gtao_directions;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Ekran uzayında taranan açısal dilim yönü sayısı (Varsayılan: 3).");
                    }

                    if (ImGui::SliderInt("Steps per Dir", &settings.gtao_steps, 2, 8))
                    {
                        m_engine_ctx.renderSettings.gtao_steps = settings.gtao_steps;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Her dilim boyunca atılan ışın yürütme örneklem adımı sayısı (Varsayılan: 4).");
                    }

                    if (ImGui::SliderFloat("Falloff Range", &settings.gtao_falloff, 0.5f, 3.0f, "%.2f"))
                    {
                        m_engine_ctx.renderSettings.gtao_falloff = settings.gtao_falloff;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Mesafe sönümleme payı. Uzak nesnelerin yapay gölge oluşturmasını engeller.");
                    }

                    if (ImGui::SliderFloat("Thickness", &settings.gtao_thickness, 0.1f, 3.0f, "%.2f m"))
                    {
                        m_engine_ctx.renderSettings.gtao_thickness = settings.gtao_thickness;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("İnce nesnelerin arkasında yapay kararma oluşmasını önleyen kalınlık toleransı.");
                    }
                }
                else // SSAO
                {
                    if (ImGui::SliderFloat("Radius", &settings.ssao_radius, 0.05f, 3.0f, "%.2f m"))
                    {
                        m_engine_ctx.renderSettings.ssao_radius = settings.ssao_radius;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Örnekleme yarıçapı. Değer büyüdükçe daha geniş alanlara gölge yayılır.");
                    }

                    if (ImGui::SliderFloat("Bias", &settings.ssao_bias, 0.001f, 0.1f, "%.3f"))
                    {
                        m_engine_ctx.renderSettings.ssao_bias = settings.ssao_bias;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Düz yüzeylerdeki gölge parazitlerini ve sivilcelenmeyi (Acne) önleme eşiği.");
                    }

                    if (ImGui::SliderFloat("Intensity", &settings.ssao_intensity, 0.1f, 5.0f, "%.2f"))
                    {
                        m_engine_ctx.renderSettings.ssao_intensity = settings.ssao_intensity;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Gölgenin koyuluk ve etki gücü çarpanı.");
                    }

                    if (ImGui::SliderInt("Sample Count", &settings.ssao_sample_count, 8, 32))
                    {
                        m_engine_ctx.renderSettings.ssao_sample_count = settings.ssao_sample_count;
                        project::project_manager::instance().save_project(m_engine_ctx);
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Yarım küre örneklem sayısı (Daha yüksek = daha pürüzsüz).");
                    }
                }

                if (ImGui::Checkbox("Edge-Preserving Bilateral Blur", &settings.ssao_blur))
                {
                    m_engine_ctx.renderSettings.ssao_blur = settings.ssao_blur;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Geometri kenarlarını ve silüetleri koruyarak gürültüyü gideren bilateral bulanıklaştırma.");
                }

                if (ImGui::Checkbox("Visualize AO Mask (Debug)", &settings.show_ssao_debug))
                {
                    m_engine_ctx.renderSettings.show_ssao_debug = settings.show_ssao_debug;
                    project::project_manager::instance().save_project(m_engine_ctx);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Ekranda sahne yerine doğrudan hesaplanan siyah-beyaz AO gölgeleme maskesini gösterir.");
                }
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 4. GÖKYÜZÜ KUTUSU VE ARKA PLAN (SKYBOX & ENVIRONMENT)
        // ====================================================================
        if (ImGui::CollapsingHeader("Skybox & Environment", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Checkbox("Enable Skybox Cubemap", &settings.enable_skybox))
            {
                m_engine_ctx.renderSettings.enable_skybox = settings.enable_skybox;
                project::project_manager::instance().save_project(m_engine_ctx);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Renders the 360-degree skybox cubemap behind all opaque and transparent scene geometry.");
            }

            ImGui::Spacing();
            ImGui::Text("Skybox Atlas / Cubemap:");

            const bool hasSkybox = !settings.skybox_path.empty();
            const std::string displayPath = hasSkybox ? settings.skybox_path : "None (Drag Cubemap Texture Here)";
            const float availW = ImGui::GetContentRegionAvail().x;
            const float slotW = hasSkybox ? (availW - 36.0f) : availW;

            ImGui::PushStyleColor(ImGuiCol_Button, hasSkybox ? ImVec4(0.15f, 0.35f, 0.20f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button(displayPath.c_str(), ImVec2(slotW, 26.0f)))
            {
                nfdu8char_t* outPath = nullptr;
                nfdu8filteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga" } };
                if (NFD_OpenDialogU8(&outPath, filterItem, 1, nullptr) == NFD_OKAY && outPath != nullptr)
                {
                    std::string skyPathStr = outPath;
                    std::replace(skyPathStr.begin(), skyPathStr.end(), '\\', '/');
                    size_t aPos = skyPathStr.find("Assets/");
                    if (aPos != std::string::npos) {
                        skyPathStr = skyPathStr.substr(aPos);
                    }
                    const unsigned int newTex = cubemap_loader::load_atlas(outPath);
                    if (newTex != 0)
                    {
                        if (m_engine_ctx.skyboxTexture != 0) glDeleteTextures(1, &m_engine_ctx.skyboxTexture);
                        m_engine_ctx.skyboxTexture = newTex;
                        settings.skybox_path = skyPathStr;
                        settings.enable_skybox = true;
                        m_engine_ctx.renderSettings = settings;
                        project::project_manager::instance().save_project(m_engine_ctx);
                        std::cout << "[RenderingPanel] Yeni skybox atlas basariyla yuklendi ve kaydedildi: " << skyPathStr << "\n";
                    }
                    NFD_FreePathU8(outPath);
                }
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Click to browse or drag & drop a 4x3 horizontal cross cubemap atlas image.");
            }

            // Sürükle ve Bırak Desteği
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD"))
                {
                    const auto* payload = static_cast<const vesta::asset_drag_payload*>(p->Data);
                    if (payload && payload->path[0] != '\0')
                    {
                        std::string skyPathStr = payload->path;
                        std::replace(skyPathStr.begin(), skyPathStr.end(), '\\', '/');
                        size_t aPos = skyPathStr.find("Assets/");
                        if (aPos != std::string::npos) {
                            skyPathStr = skyPathStr.substr(aPos);
                        }
                        const unsigned int newTex = cubemap_loader::load_atlas(payload->path);
                        if (newTex != 0)
                        {
                            if (m_engine_ctx.skyboxTexture != 0) glDeleteTextures(1, &m_engine_ctx.skyboxTexture);
                            m_engine_ctx.skyboxTexture = newTex;
                            settings.skybox_path = skyPathStr;
                            settings.enable_skybox = true;
                            m_engine_ctx.renderSettings = settings;
                            project::project_manager::instance().save_project(m_engine_ctx);
                            std::cout << "[RenderingPanel] DragDrop ile yeni skybox yuklendi ve kaydedildi: " << skyPathStr << "\n";
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // X Kaldırma Butonu
            if (hasSkybox)
            {
                ImGui::SameLine();
                if (ImGui::Button("X##ClearSkybox", ImVec2(30.0f, 26.0f)))
                {
                    if (m_engine_ctx.skyboxTexture != 0)
                    {
                        glDeleteTextures(1, &m_engine_ctx.skyboxTexture);
                        m_engine_ctx.skyboxTexture = 0;
                    }
                    settings.skybox_path.clear();
                    settings.enable_skybox = false;
                    m_engine_ctx.renderSettings = settings;
                    project::project_manager::instance().save_project(m_engine_ctx);
                    std::cout << "[RenderingPanel] Skybox atlas kaldirildi ve proje kaydedildi.\n";
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Remove skybox cubemap.");
                }
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Fallback Clear Colors (When Skybox is Off)");
            ImGui::ColorEdit3("Scene View Background", &settings.clear_color_scene.x, ImGuiColorEditFlags_Float);
            ImGui::ColorEdit3("Game View Background", &settings.clear_color_game.x, ImGuiColorEditFlags_Float);
        }

        ImGui::Spacing();

        // ====================================================================
        // 5. ATMOSFERİK SİS (DISTANCE + HEIGHT FOG + SUN IN-SCATTERING)
        // ====================================================================
        if (ImGui::CollapsingHeader("Atmosphere & Exponential Fog", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& fog = m_engine_ctx.fog;
            bool fogChanged = false;

            fogChanged |= ImGui::Checkbox("Enable Fog (Game)", &fog.enabled);
            ImGui::SameLine();
            fogChanged |= ImGui::Checkbox("Show Fog in Scene View", &fog.show_in_scene);

            const char* fogModes[] = {
                "Exponential Distance Fog (Mesafe Sisi)",
                "Exponential Height Fog (Yükseklik + Mesafe Sisi)"
            };
            fogChanged |= ImGui::Combo("Fog Mode", &fog.mode, fogModes, IM_ARRAYSIZE(fogModes));

            fogChanged |= ImGui::ColorEdit3("Fog Color", &fog.color.x, ImGuiColorEditFlags_Float);
            fogChanged |= ImGui::Checkbox("Link Fog Color to Ambient Sky Color", &fog.link_to_ambient_sky);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Sis rengini GI Gökyüzü (Ambient Sky) rengi ile otomatik harmanlar.");
            }

            fogChanged |= ImGui::SliderFloat("Fog Density", &fog.density, 0.0001f, 0.0500f, "%.4f");

            if (fog.mode == 1)
            {
                fogChanged |= ImGui::SliderFloat("Height Falloff", &fog.height_falloff, 0.005f, 0.500f, "%.3f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Yükseklik arttıkça sisin seyrekleşme hızı. Düşük değerler sisi üst kemerlere kadar yayar.");
                }
                fogChanged |= ImGui::SliderFloat("Fog Base Height (Y)", &fog.base_height, -20.0f, 50.0f, "%.1f m");
            }

            fogChanged |= ImGui::SliderFloat("Sun Scattering Intensity", &fog.sun_scattering_intensity, 0.0f, 2.0f, "%.2f");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Güneş (Directional Light) yönüne bakarken sisin güneş rengine kayma (Mie In-Scattering) şiddeti.");
            }

            ImGui::TextDisabled("Atmospheric Presets:");
            if (ImGui::SmallButton("Sponza Courtyard Mist"))
            {
                fog.enabled = true; fog.show_in_scene = true; fog.mode = 1;
                fog.density = 0.0065f; fog.height_falloff = 0.085f; fog.base_height = 0.0f;
                fog.sun_scattering_intensity = 0.45f; fog.color = glm::vec3(0.68f, 0.76f, 0.88f);
                fogChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Morning Mist"))
            {
                fog.enabled = true; fog.mode = 1; fog.density = 0.012f;
                fog.height_falloff = 0.06f; fog.sun_scattering_intensity = 0.60f;
                fog.color = glm::vec3(0.75f, 0.82f, 0.92f);
                fogChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Dense Fog"))
            {
                fog.enabled = true; fog.mode = 0; fog.density = 0.025f;
                fog.sun_scattering_intensity = 0.20f; fog.color = glm::vec3(0.60f, 0.65f, 0.70f);
                fogChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Sunset Dusk"))
            {
                fog.enabled = true; fog.mode = 1; fog.density = 0.009f;
                fog.height_falloff = 0.07f; fog.sun_scattering_intensity = 0.85f;
                fog.color = glm::vec3(0.85f, 0.50f, 0.32f);
                fogChanged = true;
            }

            if (fogChanged)
            {
                project::project_manager::instance().save_project(m_engine_ctx);
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 5.5. DOLAYLI IŞIK (INDIRECT DIFFUSE GI: HEMISPHERE & SSGI)
        // ====================================================================
        if (ImGui::CollapsingHeader("Indirect Diffuse GI (Dolaylı Işık & SSGI)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool giChanged = false;

            giChanged |= ImGui::Checkbox("Enable Indirect GI", &settings.enable_gi);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Light probe veya bake gerektirmeden gökyüzü/zemin yarım-küre ışıması ve ekran uzayı tek-sekme SSGI ile dolaylı aydınlatma sağlar.");
            }

            if (settings.enable_gi)
            {
                const char* giMethods[] = {
                    "Hemisphere Sky/Ground Irradiance (Analitik - En Hızlı)",
                    "Hybrid SSGI + Hemisphere (Yarım Çözünürlük Tek-Bounce + Bilateral Denoise)"
                };
                giChanged |= ImGui::Combo("GI Method", &settings.gi_method, giMethods, IM_ARRAYSIZE(giMethods));

                giChanged |= ImGui::ColorEdit3("Ambient Sky Color", &settings.gi_sky_color.x, ImGuiColorEditFlags_Float);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Yukarı bakan yüzeyleri aydınlatan gökyüzü dolaylı ışık rengi.");
                }

                giChanged |= ImGui::ColorEdit3("Ambient Ground Bounce Color", &settings.gi_ground_color.x, ImGuiColorEditFlags_Float);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Aşağı bakan kemer ve tavan yüzeylerini aydınlatan sıcak zemin sekmesi rengi.");
                }

                if (settings.gi_method == 1)
                {
                    giChanged |= ImGui::SliderFloat("SSGI Bounce Intensity", &settings.gi_intensity, 0.0f, 4.0f, "%.2f");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Güneşli zeminden ve renkli perdelerden çevredeki gölgeli duvarlara seken ekran uzayı dolaylı ışık şiddeti.");
                    }

                    giChanged |= ImGui::SliderFloat("SSGI Bounce Radius", &settings.gi_radius, 0.5f, 12.0f, "%.2f m");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Dolaylı ışık sarmal örnekleme yarıçapı (Dünya uzayı metre).");
                    }

                    giChanged |= ImGui::SliderInt("SSGI Sample Count", &settings.gi_sample_count, 4, 16);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Piksel başına ekran uzayı ışın/örneklem sayısı (Varsayılan: 8).");
                    }
                }
            }

            if (giChanged)
            {
                m_engine_ctx.renderSettings = settings;
                project::project_manager::instance().save_project(m_engine_ctx);
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 6. STANDART PBR, HDR TON EŞLEME VE IBL (PBR, TONE MAPPING & IBL)
        // ====================================================================
        if (ImGui::CollapsingHeader("Standard PBR Pipeline, Tone Mapping & IBL", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool pbrChanged = false;

            const char* toneMapModes[] = {
                "None (Linear Clamp)",
                "Reinhard (Extended White Point)",
                "ACES Filmic (Narkowicz HDR - Recommended)",
                "AgX (Filmic Highlight Desaturation)",
                "Uncharted 2 / Hable Filmic"
            };
            pbrChanged |= ImGui::Combo("Tone Mapping Operator", &settings.pbr_tone_mapping, toneMapModes, IM_ARRAYSIZE(toneMapModes));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("RGBA16F doğrusal HDR tamponundaki sahne ışığını sRGB dönüşümü öncesinde ekrana eşleyen operatör.");
            }

            pbrChanged |= ImGui::SliderFloat("Exposure (EV)", &settings.pbr_exposure, -3.0f, 3.0f, "%+.2f EV");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Pozlama değeri (2^EV çarpanı). Güneşli alanların patlamasını önlemek veya karanlık sahneyi açmak için kullanılır.");
            }

            if (settings.pbr_tone_mapping == 1 || settings.pbr_tone_mapping == 4)
            {
                pbrChanged |= ImGui::SliderFloat("Tone Map White Point", &settings.pbr_white_point, 1.0f, 12.0f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Reinhard ve Uncharted 2 operatörlerinde tam beyaza kesilecek üst parlaklık eşiği.");
                }
            }

            pbrChanged |= ImGui::SliderFloat("Diffuse IBL / Ambient Intensity", &settings.pbr_ambient_intensity, 0.0f, 2.0f, "%.2f");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Dolaylı yayılı (Diffuse IBL / Hemisphere) ortam aydınlatması şiddeti (0.0 - 2.0).");
            }

            pbrChanged |= ImGui::SliderFloat("Specular IBL Intensity", &settings.pbr_ibl_intensity, 0.0f, 2.0f, "%.2f");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Gökyüzü küp haritası aynasal çevresel yansıma (Specular IBL) çarpanı (Lagarde Specular Occlusion ile korunur).");
            }

            pbrChanged |= ImGui::SliderFloat("Direct Specular Highlight", &settings.pbr_specular_intensity, 0.0f, 3.0f, "%.2fx");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Cook-Torrance GGX doğrudan güneş/ışık parlama (Specular) şiddeti çarpanı.");
            }

            pbrChanged |= ImGui::SliderFloat("Global Roughness Scale", &settings.pbr_roughness_multiplier, 0.1f, 2.0f, "%.2fx");
            pbrChanged |= ImGui::SliderFloat("Global Metallic Scale", &settings.pbr_metallic_multiplier, 0.0f, 2.0f, "%.2fx");
            pbrChanged |= ImGui::SliderFloat("Global Normal Map Strength", &settings.pbr_normal_strength, 0.0f, 3.0f, "%.2fx");

            ImGui::Spacing();
            ImGui::TextDisabled("PBR & HDR Presets:");
            if (ImGui::SmallButton("Sponza Natural GI & ACES (Recommended)"))
            {
                settings.pbr_tone_mapping = 2;
                settings.pbr_exposure = 0.0f;
                settings.pbr_white_point = 4.0f;
                settings.pbr_ambient_intensity = 0.22f;
                settings.pbr_ibl_intensity = 0.15f;
                settings.pbr_specular_intensity = 1.0f;
                settings.pbr_roughness_multiplier = 1.0f;
                settings.pbr_metallic_multiplier = 1.0f;
                settings.pbr_normal_strength = 1.0f;
                settings.enable_gi = true;
                settings.gi_method = 1;
                settings.gi_intensity = 1.15f;
                pbrChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("AgX Cinematic"))
            {
                settings.pbr_tone_mapping = 3;
                settings.pbr_exposure = 0.15f;
                settings.pbr_ambient_intensity = 0.24f;
                settings.pbr_ibl_intensity = 0.18f;
                settings.pbr_specular_intensity = 1.0f;
                settings.enable_gi = true;
                settings.gi_method = 1;
                pbrChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Deep Horror Shadow"))
            {
                settings.pbr_tone_mapping = 4;
                settings.pbr_exposure = -0.35f;
                settings.pbr_white_point = 4.5f;
                settings.pbr_ambient_intensity = 0.09f;
                settings.pbr_ibl_intensity = 0.06f;
                settings.pbr_specular_intensity = 1.15f;
                pbrChanged = true;
            }

            if (pbrChanged)
            {
                m_engine_ctx.renderSettings = settings;
                project::project_manager::instance().save_project(m_engine_ctx);
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 6.5. KENAR YUMUŞATMA (ANTI-ALIASING: OFF / FXAA / TAA)
        // ====================================================================
        if (ImGui::CollapsingHeader("Anti-Aliasing (FXAA 3.11 & Temporal AA)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool aaChanged = false;

            const char* aaModes[] = {
                "Off (Kapalı)",
                "FXAA 3.11 (LDR Post-ToneMap Kenar Yumuşatma)",
                "TAA (Halton Jitter + YCoCg Variance Clip + Sharpen)"
            };
            aaChanged |= ImGui::Combo("Anti-Aliasing Mode", &settings.aa_mode, aaModes, IM_ARRAYSIZE(aaModes));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("FXAA: Hızlı tek-kare LDR kenar yumuşatma.\nTAA: 8-örnekli Halton alt-piksel projeksiyon kaydırması, geçmiş tamponu reprojeksiyonu ve YCoCg varyans kırpma.");
            }

            if (settings.aa_mode == 2)
            {
                aaChanged |= ImGui::SliderFloat("TAA History Blend / Feedback", &settings.taa_blend, 0.50f, 0.96f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Geçmiş karelerin ağırlığı (Varsayılan: 0.88). Daha yüksek değerler daha pürüzsüz kenarlar sağlar, düşük değerler hareket izini (ghosting) azaltır.");
                }

                aaChanged |= ImGui::SliderFloat("TAA Sharpen", &settings.taa_sharpen, 0.0f, 1.0f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("TAA zamansal birleştirme sonrası kontrast duyarlı keskinleştirme (CAS) miktarı.");
                }
            }

            if (aaChanged)
            {
                m_engine_ctx.renderSettings = settings;
                project::project_manager::instance().save_project(m_engine_ctx);
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 7. PS1 RETRO HORROR PIPELINE
        // ====================================================================
        if (ImGui::CollapsingHeader("PS1 Retro Horror Pipeline", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enable PS1 Retro Mode", &settings.enable_ps1_mode);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Enables PlayStation 1 era graphics emulation (vertex jitter, affine texture swimming, ordered dithering, color quantization).");
            }

            if (settings.enable_ps1_mode)
            {
                ImGui::Indent();

                // Vertex Snapping (Jitter/Wobble)
                ImGui::SliderFloat("Vertex Snapping Grid", &settings.ps1_vertex_snapping, 60.0f, 640.0f, "%.0f px");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Simulates lack of floating-point sub-pixel precision on PS1 GTE. Lower values increase vertex wobble/jitter (120-240 is authentic). Set to 0 to disable.");
                }

                // Quick buttons for snapping resolution
                if (ImGui::SmallButton("120p (Heavy Jitter)")) { settings.ps1_vertex_snapping = 120.0f; }
                ImGui::SameLine();
                if (ImGui::SmallButton("160p (Authentic Low)")) { settings.ps1_vertex_snapping = 160.0f; }
                ImGui::SameLine();
                if (ImGui::SmallButton("240p (Standard PS1)")) { settings.ps1_vertex_snapping = 240.0f; }
                ImGui::SameLine();
                if (ImGui::SmallButton("320p (Crisp)")) { settings.ps1_vertex_snapping = 320.0f; }

                ImGui::Spacing();

                // Resolution Downscaling (Chunky Pixel / Retro Scaler)
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "PS1 Resolution Downscaling (Pixelation)");
                ImGui::Checkbox("Enable Low-Res Downscale", &settings.ps1_downscale);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Renders the scene at authentic low PS1 hardware resolution and upscales with nearest-neighbor filtering for chunky pixels.");
                }

                if (settings.ps1_downscale)
                {
                    const char* resPresets[] = {
                        "240p PS1 (Free Aspect Uyumlu / En-Boy Korunur)",
                        "320x240 Sabit (PS1 4:3 Klasik)",
                        "426x240 Sabit (PS1 16:9 Genis Ekran)",
                        "480p Hi-Res PS1 (Free Aspect Uyumlu)",
                        "Dinamik 0.25x (Viewport Ceyrek Boyut)",
                        "Ozel Cozunurluk (Custom Width x Height)..."
                    };
                    ImGui::Combo("Resolution Preset", &settings.ps1_resolution_preset, resPresets, IM_ARRAYSIZE(resPresets));

                    if (settings.ps1_resolution_preset == 5)
                    {
                        ImGui::DragInt("Custom Width", &settings.ps1_custom_width, 2.0f, 64, 1920);
                        ImGui::DragInt("Custom Height", &settings.ps1_custom_height, 2.0f, 64, 1080);
                    }

                    ImGui::TextDisabled("Game Target: %d x %d (Upscaled via GL_NEAREST, Editor: 1:1 Full)", 
                        m_engine_ctx.gameTarget.width, 
                        m_engine_ctx.gameTarget.height);
                }
                ImGui::Spacing();

                // Affine Texture Mapping
                ImGui::Checkbox("Affine Texture Warping (No W-Divide)", &settings.ps1_affine_texture_mapping);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Simulates PS1 hardware lack of perspective-correct texture interpolation, causing textures to swim and distort on angled polygons.");
                }

                ImGui::Spacing();

                // Color Depth / Posterization
                ImGui::SliderFloat("Color Depth (Levels/Channel)", &settings.ps1_color_depth, 4.0f, 64.0f, "%.0f levels");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Quantizes RGB color precision. 32 levels = 15-bit RGB555 (Original PS1 hardware standard). 16 levels = 12-bit retro.");
                }

                // Dithering
                ImGui::Checkbox("Bayer 4x4 Ordered Dithering", &settings.ps1_dithering);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Applies authentic 4x4 Bayer pattern dithering to break color banding in low-bit-depth environments.");
                }

                if (settings.ps1_dithering)
                {
                    ImGui::SliderFloat("Dither Intensity", &settings.ps1_dither_strength, 0.1f, 2.0f, "%.2fx");
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.0f), "Retro Horror Atmosphere Presets:");

                if (ImGui::Button("Silent Hill Fog Preset"))
                {
                    settings.enable_ps1_mode = true;
                    settings.ps1_vertex_snapping = 200.0f;
                    settings.ps1_affine_texture_mapping = true;
                    settings.ps1_color_depth = 32.0f;
                    settings.ps1_dithering = true;
                    settings.ps1_dither_strength = 1.0f;
                    auto& fog = m_engine_ctx.fog;
                    fog.enabled = true;
                    fog.show_in_scene = true;
                    fog.density = 0.032f;
                    fog.color = glm::vec3(0.35f, 0.38f, 0.40f);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Dense eerie gray fog with standard PS1 15-bit color dithering and vertex wobble.");
                }

                ImGui::SameLine();
                if (ImGui::Button("Resident Evil Dark Preset"))
                {
                    settings.enable_ps1_mode = true;
                    settings.ps1_vertex_snapping = 240.0f;
                    settings.ps1_affine_texture_mapping = true;
                    settings.ps1_color_depth = 32.0f;
                    settings.ps1_dithering = true;
                    settings.ps1_dither_strength = 1.2f;
                    auto& fog = m_engine_ctx.fog;
                    fog.enabled = true;
                    fog.show_in_scene = true;
                    fog.density = 0.015f;
                    fog.color = glm::vec3(0.02f, 0.02f, 0.03f);
                    settings.clear_color_scene = glm::vec3(0.01f, 0.01f, 0.02f);
                    settings.clear_color_game = glm::vec3(0.01f, 0.01f, 0.02f);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Near pitch black darkness with high-contrast dithered lighting.");
                }

                ImGui::SameLine();
                if (ImGui::Button("Disable PS1 Mode"))
                {
                    settings.enable_ps1_mode = false;
                }

                ImGui::Unindent();
            }
        }

        ImGui::Spacing();

        // ====================================================================
        // 7. MOTOR GRAFİK YETENEKLERİ ÖZETİ (CAPABILITIES SUMMARY)
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

