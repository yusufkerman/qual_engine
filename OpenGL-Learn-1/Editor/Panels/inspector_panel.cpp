#include <glad/glad.h> // 🔥 Kalan include krizlerini önlemek için her şeyden önce GLAD!

#include "inspector_panel.h"
#include "../../ECS/registry.h"
#include <imgui.h>

// Context, Pipeline ve Varlık Tanımları
#include "../../Engine/engine_context.h"   
#include "../../Engine/asset_pipeline.h"   
#include "../../Renderer/material.h"       
#include "../../Externals/nfd/nfd.hpp"
#include "../../Externals/nfd/nfd.h"
#include <iostream>

void inspector_panel::render(scene& scene, int selectedEntity, EngineContext& ctx)
{
    auto& reg = scene.get_registry();

    ImGui::Begin("Inspector");

    if (selectedEntity == -1)
    {
        ImGui::Text("No entity selected");
        ImGui::End();
        return;
    }

    entity e = (entity)selectedEntity;

    ImGui::Text("Entity %d", selectedEntity);
    ImGui::Separator();

    // =========================================================
    // ADD COMPONENT BUTTON (Tüm Bileşenler Dahil)
    // =========================================================
    if (ImGui::Button("Add Component"))
        ImGui::OpenPopup("add_component_popup");

    if (ImGui::BeginPopup("add_component_popup"))
    {
        if (!reg.has_component<transform_component>(e)) {
            if (ImGui::MenuItem("Transform")) reg.add_component<transform_component>(e);
        }
        if (!reg.has_component<camera_component>(e)) {
            if (ImGui::MenuItem("Camera")) reg.add_component<camera_component>(e);
        }
        if (!reg.has_component<light_component>(e)) {
            if (ImGui::MenuItem("Light")) reg.add_component<light_component>(e);
        }
        if (!reg.has_component<mesh_component>(e)) {
            if (ImGui::MenuItem("Mesh Renderer")) reg.add_component<mesh_component>(e);
        }
        if (!reg.has_component<material_component>(e)) {
            if (ImGui::MenuItem("Material")) reg.add_component<material_component>(e);
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();

    // =========================================================
    // TRANSFORM
    // =========================================================
    if (reg.has_component<transform_component>(e))
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* t = reg.get_component<transform_component>(e);

            ImGui::DragFloat3("Position", &t->position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);
            ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
        }
    }

    // =========================================================
    // CAMERA
    // =========================================================
    if (reg.has_component<camera_component>(e))
    {
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* c = reg.get_component<camera_component>(e);

            ImGui::DragFloat("FOV", &c->fov, 1.0f);
            ImGui::DragFloat("Near", &c->nearPlane, 0.01f);
            ImGui::DragFloat("Far", &c->farPlane, 1.0f);
        }
    }

    // =========================================================
    // LIGHT
    // =========================================================
    if (reg.has_component<light_component>(e))
    {
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* l = reg.get_component<light_component>(e);

            ImGui::ColorEdit3("Color", &l->color.x);
            ImGui::DragFloat("Intensity", &l->intensity, 0.1f);
            ImGui::DragFloat("Range", &l->range, 1.0f);
            ImGui::DragFloat("Inner Cone", &l->inner_cone, 0.5f, 0.0f, 89.0f);
            ImGui::DragFloat("Outer Cone", &l->outer_cone, 0.5f, 0.0f, 90.0f);

            const char* typeNames[] = { "Directional", "Point", "Spot" };
            int typeIndex = (int)l->type;
            if (ImGui::Combo("Type", &typeIndex, typeNames, 3))
                l->type = (light_type)typeIndex;
        }
    }

    // =========================================================
    // MESH RENDERER (YENİ)
    // =========================================================
    if (reg.has_component<mesh_component>(e))
    {
        if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* meshComp = reg.get_component<mesh_component>(e);

            ImGui::Text("Mesh ID: %u", meshComp->mesh_id);
            ImGui::Text("Loaded File: %s", meshComp->model_path.c_str());

            // Dinamik Model Yükleme Tetikleyicisi
            if (ImGui::Button("Load New Model (.obj)"))
            {
                NFD::UniquePath outPath;

                // Sadece .obj uzantılı dosyaları göstermek için filtre hazırlıyoruz
                nfdfilteritem_t filterItem[1] = { { "Wavefront OBJ", "obj" } };

                // 📂 Windows orijinal Dosya Seçme Penceresini açıyoruz!
                nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1, nullptr);

                if (result == NFD_OKAY)
                {
                    // Kullanıcının bilgisayardan seçtiği dosyanın tam yolunu alıyoruz (Örn: "D:/Modeller/Araba.obj")
                    std::string selectedPath = outPath.get();

                    // Senin o meşhur pipeline fonksiyonunu tetikliyoruz
                    auto importResult = asset_pipeline::import_model(ctx, selectedPath);

                    if (!importResult.mesh_ids.empty())
                    {
                        // Seçilen modelin ID'sini ve yolunu bileşene (component) yazıyoruz
                        meshComp->mesh_id = importResult.mesh_ids.front();
                        meshComp->model_path = selectedPath;

                        // Görünürlük / Culling (Frustum) ayarları için bounds güncellemesi
                        if (!reg.has_component<bounds_component>(e))
                            reg.add_component<bounds_component>(e);

                        auto* bounds = reg.get_component<bounds_component>(e);
                        bounds->localCenter = importResult.boundsCenter;
                        bounds->localRadius = importResult.boundsRadius;
                    }
                }
                else if (result == NFD_CANCEL)
                {
                    // Kullanıcı pencereyi "İptal" diyerek kapattıysa hiçbir şey yapmaya gerek yok
                }
                else
                {
                    // Sıra dışı bir hata oluştuysa konsola yazdır
                    std::cout << "[NFD Error] " << NFD_GetError() << std::endl;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Remove##Mesh")) {
                // İleride component silmek istersen burayı doldurabilirsin
            }
        }
    }

    // =========================================================
    // MATERIAL (YENİ)
    // =========================================================
    if (reg.has_component<material_component>(e))
    {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* matComp = reg.get_component<material_component>(e);

            // Generic asset_manager'dan materyali çekiyoruz
            material* mat = ctx.assetManager.try_get<material>(matComp->material_id);

            if (mat)
            {
                ImGui::Text("Material ID: %u", matComp->material_id);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Shared Material - Edits affect all clones)");

                // 🔥 Instancing'i bozmamak için bağımsızlaştırma butonu
                if (ImGui::Button("Make Unique (Clone Material)"))
                {
                    material clonedMat = *mat;
                    asset_id newMatID = ctx.assetManager.add<material>(clonedMat);
                    matComp->material_id = newMatID;
                    mat = ctx.assetManager.try_get<material>(newMatID);
                }

                ImGui::Separator();

                // 🎨 Renk ve Değer Ayarları
                ImGui::ColorEdit3("Base Color", &mat->base_color.x);
                ImGui::SliderFloat("Normal Strength", &mat->normal_strength, 0.0f, 3.0f);

                // 🪵 YENİ: TEXTURE TILING (SCALE) SLIDER'I
                // Bu değer doğrudan shader'daki u_TextureScale uniform'una gidecek.
                // Değişim hızını 0.05f yaparak hassas ayar yapabilmeni sağlıyoruz. Minimum 0.01f olmalı ki doku yok olmasın.
                ImGui::DragFloat2("Texture Tiling", &mat->texture_scale.x, 0.05f, 0.01f, 100.0f, "%.2f");

                // Kullanıcıya kolaylık olsun diye hızlıca 1.0'e sıfırlama butonu
                ImGui::SameLine();
                if (ImGui::Button("Reset")) {
                    mat->texture_scale = glm::vec2(1.0f);
                }

                ImGui::Separator();

                // 🖼️ 1. DIFFUSE TEXTURE SEÇİCİ
                ImGui::Text("Diffuse Texture ID (Manager): %u", mat->diffuse_texture);
                if (ImGui::Button("Load Diffuse Texture##NFD"))
                {
                    NFD::UniquePath outPath;
                    nfdfilteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga,bmp" } };

                    nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1, nullptr);

                    if (result == NFD_OKAY)
                    {
                        std::string texturePath = outPath.get();
                        asset_id texID = asset_pipeline::import_texture(ctx, texturePath);
                        if (texID != 0)
                        {
                            mat->diffuse_texture = texID;
                        }
                    }
                }

                ImGui::Separator();

                // 🗺️ 2. NORMAL MAP SEÇİCİ
                ImGui::Text("Normal Map ID (Manager): %u", mat->normal_texture);
                if (ImGui::Button("Load Normal Map##NFD"))
                {
                    NFD::UniquePath outPath;
                    nfdfilteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga,bmp" } };

                    nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1, nullptr);

                    if (result == NFD_OKAY)
                    {
                        std::string texturePath = outPath.get();
                        asset_id texID = asset_pipeline::import_texture(ctx, texturePath);
                        if (texID != 0)
                        {
                            mat->normal_texture = texID;
                        }
                    }
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: No material assigned or invalid ID!");

                if (ImGui::Button("Create & Assign New Material"))
                {
                    material defaultMat;
                    defaultMat.shader_id = 0;
                    defaultMat.base_color = glm::vec3(1.0f);
                    defaultMat.normal_strength = 1.0f;
                    defaultMat.diffuse_texture = 0;
                    defaultMat.normal_texture = 0;
                    defaultMat.texture_scale = glm::vec2(1.0f); // Varsayılan ölçek 1.0

                    matComp->material_id = ctx.assetManager.add<material>(defaultMat);
                }
            }
        }
    }

    ImGui::End();
}