/**
 * @file mesh_component_ui.h
 * @brief Statik veya dinamik kafes (mesh) bileşenleri için editör kullanıcı arayüzü yönetim sistemi.
 *
 * Bu dosya; sahnede çizilecek 3D modellerin geometrik verilerinin, dosya yollarının takibini ve
 * Native File Dialog (NFD) aracılığıyla diskten yeni modellerin (.obj) dinamik olarak yüklenmesini sağlar.
 * Ayrıca yüklenen modelin çevreleme hacmi (Bounding Box/Sphere) verilerini otomatik günceller.
 */

#pragma once
#include "component_ui_registry.h"
#include "../../ECS/registry.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Externals/nfd/nfd.hpp"
#include "../../Externals/nfd/nfd.h"
#include <imgui.h>
#include <iostream>

 /**
  * @brief Seçili varlığın (entity) kafes (mesh) verilerini ImGui üzerinde çizip günceller.
  *
  * @param e Üzerinde işlem yapılan ve arayüzü çizilecek olan ECS varlığı (Entity).
  * @param reg ECS veri tabanı ve bileşen yöneticisi (Registry) referansı.
  * @param engineCtx Motorun genel çalışma zamanı bağlamı (EngineContext) - Model yükleme hattı erişimi için.
  * @param editorCtx Editörün panel odakları ve arayüz durumu (EditorContext).
  */
inline void render_mesh_ui(entity e, registry& reg, EngineContext& engineCtx, EditorContext& editorCtx) {
    auto* meshComp = reg.get_component<mesh_component>(e);
    if (!meshComp) return;

    ImGui::Text("Mesh ID: %u", meshComp->mesh_id);
    ImGui::Text("Loaded File: %s", meshComp->model_path.c_str());

    // ---------------------------------------------------------
    // MESH SOURCE (MODEL / CUBE / SPHERE)
    // ---------------------------------------------------------
    static const char* items[] = { "Model", "Cube", "Sphere" };
    int current = (int)meshComp->source;

    if (ImGui::Combo("Mesh Source", &current, items, IM_ARRAYSIZE(items)))
    {
        meshComp->source = (mesh_source_type)current;

        if (meshComp->source == mesh_source_type::primitive_cube)
        {
            auto res = asset_pipeline::create_cube_mesh(engineCtx);
            if (!res.mesh_ids.empty())
                meshComp->mesh_id = res.mesh_ids[0];

            meshComp->model_path = "internal://primitive_cube";
        }

        if (meshComp->source == mesh_source_type::primitive_sphere)
        {
            auto res = asset_pipeline::create_sphere_mesh(engineCtx);
            if (!res.mesh_ids.empty())
                meshComp->mesh_id = res.mesh_ids[0];

            meshComp->model_path = "internal://primitive_sphere";
        }
    }

    // ---------------------------------------------------------
    // LOAD MODEL
    // ---------------------------------------------------------
    if (ImGui::Button("Load Model (.obj)"))
    {
        NFD::UniquePath outPath;
        nfdfilteritem_t filterItem[1] = { { "Wavefront OBJ", "obj" } };

        nfdresult_t result = NFD::OpenDialog(outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY)
        {
            auto importResult = asset_pipeline::import_model(engineCtx, outPath.get());

            if (!importResult.mesh_ids.empty())
            {
                meshComp->mesh_id = importResult.mesh_ids.front();
                meshComp->model_path = outPath.get();
                meshComp->source = mesh_source_type::model;

                if (!reg.has_component<bounds_component>(e))
                    reg.add_component<bounds_component>(e);

                auto* bounds = reg.get_component<bounds_component>(e);
                bounds->localCenter = importResult.boundsCenter;
                bounds->localRadius = importResult.boundsRadius;
            }
        }
        else if (result == NFD_ERROR)
        {
            std::cout << "[NFD Error] " << NFD_GetError() << std::endl;
        }
    }

    // ---------------------------------------------------------
    // REMOVE MODEL (FALLBACK TO CUBE)
    // ---------------------------------------------------------
    if (ImGui::Button("Remove Model"))
    {
        meshComp->source = mesh_source_type::primitive_cube;
        meshComp->model_path = "internal://primitive_cube";

        auto res = asset_pipeline::create_cube_mesh(engineCtx);
        if (!res.mesh_ids.empty())
            meshComp->mesh_id = res.mesh_ids[0];
    }
}

/**
 * @brief Belirtilen varlığa (entity) eğer sahip değilse varsayılan bir mesh bileşeni ekler.
 *
 * @param e Mesh bileşeninin ekleneceği ECS varlığı.
 * @param reg ECS veri tabanı (Registry) referansı.
 */
inline void add_mesh_component(entity e, registry& reg, EngineContext&, EditorContext&) {
    if (!reg.has_component<mesh_component>(e)) {
        reg.add_component<mesh_component>(e);
    }
}

/**
 * @brief Kafes Çizici (Mesh Renderer) arayüz alt sistemini merkezi UI kayıt yöneticisine enjekte eder.
 *
 * Derleme zamanı bağımlılık zincirini yalıtmak amacıyla harita tabanlı runtime kayıt algoritmasını tetikler.
 */
inline void register_mesh_ui_system() {
    ComponentUIInfo info;
    info.name = "Mesh Renderer";
    info.render_fn = render_mesh_ui;
    info.add_fn = add_mesh_component;

    // Tip gizleme (type-erasure) işlemi için has_component kontrol lambdası
    info.has_fn = [](entity e, const registry& reg) -> bool {
        return reg.has_component<mesh_component>(e);
        };

    // Mesh bileşenini ECS'den güvenle kaldıran remove_fn lambdası
    info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<mesh_component>(e);
        };

    // Benzersiz tip imzasını (hash_code) anahtar olarak kullanarak sisteme kayıt işlemini tamamlıyoruz
    component_ui_registry::register_component(typeid(mesh_component).hash_code(), info);
}