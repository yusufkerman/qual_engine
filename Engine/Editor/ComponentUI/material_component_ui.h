/**
 * @file material_component_ui.h
 * @brief Materyal bileşeni (material_component) için editör kullanıcı arayüzü yönetim sistemi.
 *
 * Bu dosya; sahnede render edilen nesnelerin renk, normal haritası kuvveti, doku ölçeklemesi (tiling)
 * ve doku dosyalarının (Diffuse/Normal Map) diskten dinamik olarak NFD ile yüklenmesi gibi
 * görsel parametrelerinin ImGui panellerinde manipüle edilmesini ve merkezi arayüz sistemine kaydedilmesini sağlar.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Renderer/material.h"
#include "../../Externals/nfd/nfd.hpp"
#include "../../Externals/nfd/nfd.h"
#include <imgui.h>

 /**
  * @brief Seçili varlığın (entity) materyal bileşeni verilerini ImGui üzerinde çizip günceller.
  *
  * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı (EngineContext) - Varlık Yöneticisi erişimi için.
  * @param editorCtx Editörün panel odakları ve arayüz durumu (EditorContext).
  */
inline void render_material_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    // Görsel bütünlük için katlanabilir bir başlık (Collapsing Header) oluşturuyoruz
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {

        // Varlığa ait materyal bileşeninin ham göstergesini güvenli bir şekilde çekiyoruz
        auto* matComp = reg.get_component<material_component>(e);
        if (!matComp) return;

        // Varlık yöneticisinden (AssetManager) materyalin asıl örneğini (instance) ID ile çekiyoruz
        material* mat = engineCtx.assetManager.try_get<material>(matComp->material_id);

        if (mat) {
            ImGui::Text("Material ID: %u", matComp->material_id);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Shared Material - Edits affect all clones)");

            // ---------------------------------------------------------
            // 1. MATERYALİ BAĞIMSIZLAŞTIRMA (MAKE UNIQUE)
            // ---------------------------------------------------------
            // Paylaşılan materyali klonlayarak bu nesneye özel (unique) hale getirir.
            // Böylece yapılan renk/doku değişiklikleri bu materyali kullanan diğer nesneleri etkilemez.
            if (ImGui::Button("Make Unique (Clone Material)")) {
                material clonedMat = *mat; // Materyal verisini kopyala
                asset_id newMatID = engineCtx.assetManager.add<material>(clonedMat); // Yeni asset olarak kaydet
                matComp->material_id = newMatID; // Nesneye yeni ID'yi ata
                mat = engineCtx.assetManager.try_get<material>(newMatID); // Göstergeyi güncelle
            }

            ImGui::Separator();

            // Temel Renk (Albedo/Base Color) Ayarı
            ImGui::ColorEdit3("Base Color", &mat->base_color.x);

            // Normal Haritası Etki Şiddeti (Normal Strength)
            ImGui::SliderFloat("Normal Strength", &mat->normal_strength, 0.0f, 3.0f);

            // Doku Döşeme Ölçeği (Texture Tiling / Scale) - Hassas kaydırma hızı: 0.05
            ImGui::DragFloat2("Texture Tiling", &mat->texture_scale.x, 0.05f, 0.01f, 100.0f, "%.2f");

            ImGui::SameLine();
            // Tiling değerini hızlıca varsayılan 1.0f ölçeğine sıfırlama butonu
            if (ImGui::Button("Reset")) {
                mat->texture_scale = glm::vec2(1.0f);
            }

            // ---------------------------------------------------------
            // 2. DIFFUSE TEXTURE YÜKLEME (NFD ENTEGRASYONU)
            // ---------------------------------------------------------
            ImGui::Separator();
            ImGui::Text("Diffuse Texture ID: %u", mat->diffuse_texture);
            if (ImGui::Button("Load Diffuse Texture##NFD")) {
                NFD::UniquePath outPath;
                nfdfilteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga,bmp" } };

                // İşletim sisteminin yerel dosya seçme penceresini açıyoruz
                if (NFD::OpenDialog(outPath, filterItem, 1, nullptr) == NFD_OKAY) {
                    // Seçilen görseli asset_pipeline aracılığıyla GPU'ya yükleyip ID'sini alıyoruz
                    asset_id texID = asset_pipeline::import_texture(engineCtx, outPath.get());
                    if (texID != 0) mat->diffuse_texture = texID;
                }
            }

            // ---------------------------------------------------------
            // 3. NORMAL MAP YÜKLEME (NFD ENTEGRASYONU)
            // ---------------------------------------------------------
            ImGui::Separator();
            ImGui::Text("Normal Map ID: %u", mat->normal_texture);
            if (ImGui::Button("Load Normal Map##NFD")) {
                NFD::UniquePath outPath;
                nfdfilteritem_t filterItem[1] = { { "Image Files", "png,jpg,jpeg,tga,bmp" } };

                if (NFD::OpenDialog(outPath, filterItem, 1, nullptr) == NFD_OKAY) {
                    asset_id texID = asset_pipeline::import_texture(engineCtx, outPath.get());
                    if (texID != 0) mat->normal_texture = texID;
                }
            }
        }
        else {
            // Materyal ID'sinin geçersiz veya bozuk olması durumundaki güvenlik senaryosu
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: No material assigned!");
            if (ImGui::Button("Create & Assign New Material")) {
                material defaultMat;
                defaultMat.shader_id = 0;
                defaultMat.base_color = glm::vec3(1.0f);
                defaultMat.normal_strength = 1.0f;
                defaultMat.diffuse_texture = 0;
                defaultMat.normal_texture = 0;
                defaultMat.texture_scale = glm::vec2(1.0f);

                matComp->material_id = engineCtx.assetManager.add<material>(defaultMat);
            }
        }
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse varsayılan bir materyal bileşeni ekler.
 *
 * @param e Materyal bileşeninin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_material_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<material_component>(e)) {
        reg.add_component<material_component>(e);
    }
}

/**
 * @brief Materyal (Material) arayüz alt sistemini merkezi UI kayıt yöneticisine enjekte eder.
 *
 * Derleme zamanı bağımlılıklarını izole etmek için runtime üzerinde type-erased sorgu lambdası kaydeder.
 */
inline void register_material_ui_system() {
    ComponentUIInfo info;
    info.name = "Material";
    info.render_fn = render_material_ui;
    info.add_fn = add_material_component;

    // Tip gizleme (type-erasure) işlemi için has_component kontrol lambdası
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<material_component>(e);
        };

    // Benzersiz tip imzasını (hash_code) anahtar olarak kullanarak sisteme kayıt işlemini tamamlıyoruz
    component_ui_registry::register_component(typeid(material_component).hash_code(), info);
}