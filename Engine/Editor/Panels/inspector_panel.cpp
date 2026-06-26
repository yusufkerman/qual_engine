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

void inspector_panel::render(scene& scene, int selectedEntity, EngineContext& engineCtx, EditorContext& editorCtx)
{
    auto& reg = scene.get_registry();

    ImGui::Begin("Inspector");

    // Eğer kullanıcı şu an bu panele tıkladıysa veya bu panel aktifse bağlamı güncelle
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        editorCtx.focusedPanel = FocusedPanel::Inspector;
    }

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
        if (!reg.has_component<rigidbody_component>(e)) {
            if (ImGui::MenuItem("Rigidbody")) {
                // Varsayılan olarak dynamic, 1.0 kg kütle ve 0.5 sürtünme ile başlatıyoruz
                reg.add_component<rigidbody_component>(e, rigidbody_component{ motion_type::dynamic_body, 1.0f, 0.5f });
            }
        }
        if (!reg.has_component<collider_component>(e)) {
            if (ImGui::MenuItem("Collider")) {
                // Varsayılan olarak Box ve 1x1x1 boyutlarında başlatıyoruz
                reg.add_component<collider_component>(e, collider_component{ collider_type::box, glm::vec3(1.0f) });
            }
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
                    auto importResult = asset_pipeline::import_model(engineCtx, selectedPath);

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
    // MATERIAL
    // =========================================================
    if (reg.has_component<material_component>(e))
    {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* matComp = reg.get_component<material_component>(e);

            // Generic asset_manager'dan materyali çekiyoruz
            material* mat = engineCtx.assetManager.try_get<material>(matComp->material_id);

            if (mat)
            {
                ImGui::Text("Material ID: %u", matComp->material_id);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Shared Material - Edits affect all clones)");

                // 🔥 Instancing'i bozmamak için bağımsızlaştırma butonu
                if (ImGui::Button("Make Unique (Clone Material)"))
                {
                    material clonedMat = *mat;
                    asset_id newMatID = engineCtx.assetManager.add<material>(clonedMat);
                    matComp->material_id = newMatID;
                    mat = engineCtx.assetManager.try_get<material>(newMatID);
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
                        asset_id texID = asset_pipeline::import_texture(engineCtx, texturePath);
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
                        asset_id texID = asset_pipeline::import_texture(engineCtx, texturePath);
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

                    matComp->material_id = engineCtx.assetManager.add<material>(defaultMat);
                }
            }
        }
    }
    // =========================================================
    // RIGIDBODY / JOLT PHYSICS (YENİ)
    // =========================================================
    if (reg.has_component<rigidbody_component>(e))
    {
        if (ImGui::CollapsingHeader("Rigidbody (Jolt Physics)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* rb = reg.get_component<rigidbody_component>(e);

            // Jolt BodyID bilgisini debug amaçlı gösterelim
            if (!rb->body_id.IsInvalid()) {
                ImGui::Text("Jolt Body ID: %u", rb->body_id.GetIndex());
            }
            else {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Editor Mode (Fizik Pasif)");
            }

            ImGui::Separator();

            // 1. Motion Type Seçimi (Static, Kinematic, Dynamic)
            const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };
            int motionIndex = (int)rb->motion; // Component içindeki değişken adın (motion/motion_type)

            if (ImGui::Combo("Motion Type", &motionIndex, motionNames, 3))
            {
                rb->motion = (motion_type)motionIndex;

                // ⚠️ EĞER OYUN OYNANIRKEN DEĞİŞTİRİLİRSE:
                // Jolt'un arkadaki body tipini de canlı değiştirmek gerekir. 
                // Şimdilik editör modunda güvenle değişir, PLAY modunda kökten yeniden kurulur.
            }

            // 2. Kütle ve Sürtünme Ayarları
            // Eğer nesne static ise kütle ayarını kapatmak (Disable) mantıklıdır
            if (rb->motion == motion_type::static_body) {
                ImGui::BeginDisabled();
                float dummyMass = 0.0f;
                ImGui::DragFloat("Mass (Kg)", &dummyMass, 0.0f);
                ImGui::EndDisabled();
            }
            else {
                ImGui::DragFloat("Mass (Kg)", &rb->mass, 0.05f, 0.01f, 10000.0f, "%.2f");
            }

            // Sürtünme katsayısı genellikle 0 ile 1 arasındadır
            ImGui::SliderFloat("Friction", &rb->friction, 0.0f, 1.0f, "%.2f");

            ImGui::Separator();

            // Component Kaldırma Butonu
            if (ImGui::Button("Remove Rigidbody"))
            {
                // Eğer oyun oynanırken kaldırılırsa Jolt dünyasından da silmek gerekir
                if (engineCtx.editor.mode == EngineMode::PLAY && !rb->body_id.IsInvalid()) {
                    auto* jolt_system = engineCtx.physicsSystem.get_jolt_system();
                    jolt_system->GetBodyInterface().RemoveBody(rb->body_id);
                    jolt_system->GetBodyInterface().DestroyBody(rb->body_id);
                }
                reg.remove_component<rigidbody_component>(e);
            }
        }
    }
    // =========================================================
    // COLLIDER COMPONENT (YENİ)
    // =========================================================
    if (reg.has_component<collider_component>(e))
    {
        if (ImGui::CollapsingHeader("Collider (Jolt Shape)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto* col = reg.get_component<collider_component>(e);

            // 1. Collider Tipi Seçimi (Box, Sphere)
            const char* colliderNames[] = { "Box", "Sphere" };
            int typeIndex = (int)col->type;

            if (ImGui::Combo("Collider Type", &typeIndex, colliderNames, 2))
            {
                col->type = (collider_type)typeIndex;

                // Tip değiştiğinde kullanıcıyı şaşırtmamak için extents değerini sıfırlamayalım, 
                // x değerini doğrudan radius kabul edeceğiz.
            }

            ImGui::Separator();

            // 2. Tipe Göre Dinamik Boyutlandırma UI Kontrolleri
            if (col->type == collider_type::box)
            {
                // Unity'deki Box Collider "Size" mantığı (Aslında Jolt Half-Extents bekler, 
                // yani merkezin kenara olan uzaklığı. Buraya not düşmek editör kolaylığı sağlar)
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Box Dimensions (Half-Extents)");
                ImGui::DragFloat3("Extents", &col->extents.x, 0.05f, 0.01f, 100.0f, "%.2f");
            }
            else if (col->type == collider_type::sphere)
            {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Sphere Dimensions");

                // Küre için sadece tek bir yarıçap (Radius) yeterlidir, extents.x bileşenini kullanalım
                ImGui::DragFloat("Radius", &col->extents.x, 0.05f, 0.01f, 100.0f, "%.2f");

                // Yarıçap değiştiğinde y ve z'yi de senkron tutalım ki veri temiz kalsın
                col->extents.y = col->extents.x;
                col->extents.z = col->extents.x;
            }

            ImGui::Separator();

            // Component Kaldırma Butonu
            if (ImGui::Button("Remove Collider"))
            {
                // Güvenlik önlemi: Eğer PLAY modunda collider silinirse, Jolt'un patlamaması için 
                // Rigidbody'nin de durumunu güncellemek gerekir. Editor modunda sorunsuz silinir.
                reg.remove_component<collider_component>(e);
            }
        }
    }

    ImGui::End();
}