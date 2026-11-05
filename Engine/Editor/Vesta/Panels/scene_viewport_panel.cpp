/**
 * @file scene_viewport_panel.cpp
 * @brief Sahne görünümü (Scene Viewport) panelinin ImGui ile çizim ve etkileşimli gizmo katmanı işlemlerini barındırır.
 */

#include "scene_viewport_panel.h"
#include "../../../Engine/engine_context.h"
#include "../../../Engine/Assets/asset_loader.h"
#include "../../../Engine/asset_pipeline.h"
#include "../../../Engine/fly_camera.h"
#include "../../../Core/path_utils.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"
#include "../Inspector/Components/ui_components_ui.h"
#include "../Services/asset_browser_system.h"
#include "../../../Editor/editor_camera.h"
#include <filesystem>
#include <imgui.h>

namespace vesta {

    scene_viewport_panel::scene_viewport_panel(
        scene_viewport_view_model& vm,
        editor_viewport_state& viewportState,
        EngineContext* engineCtx,
        EditorContext* editorCtx)
        : m_vm(vm)
        , m_viewport_state(viewportState)
        , m_engine_ctx(engineCtx)
        , m_editor_ctx(editorCtx)
    {
    }

    void scene_viewport_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);

        auto& scene_state = m_viewport_state.scene();

        // Girdi (Input), kısayol ve fly-cam kontrolleri için odak/imleç durumlarını güncelle
        scene_state.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        scene_state.hovered = ImGui::IsWindowHovered(ImGuiFocusedFlags_RootAndChildWindows);

        ImGui::PushID("SceneViewport");

        // Render alanının pencereye tam oturması için kenar boşluklarını sıfırla
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 1.0f && avail.y > 1.0f)
        {
            scene_state.size = { avail.x, avail.y };
        }

        // FBO yeniden boyutlandığında doku kimliği değişebileceğinden güncel tut
        if (m_engine_ctx && m_engine_ctx->editorTarget.tex != 0)
        {
            m_vm.set_texture_id(m_engine_ctx->editorTarget.tex);
        }

        unsigned int tex = m_vm.get_texture_id();
        if (avail.x > 0.0f && avail.y > 0.0f && tex != 0)
        {
            ImVec2 cursor = ImGui::GetCursorScreenPos();

            // Gizmo ve raycast/picking işlemleri için sol üst ekran koordinatını kaydet
            scene_state.position = { cursor.x, cursor.y };

            // Çizimin panel sınırları dışına taşmasını engellemek için kırpma (clipping) uygula
            ImGui::GetWindowDrawList()->PushClipRect(
                cursor,
                ImVec2(cursor.x + avail.x, cursor.y + avail.y),
                true
            );

            // OpenGL UV koordinatlarını (0,1 -> 1,0) ters çevirerek FBO dokusunu bas
            ImGui::Image(
                (ImTextureID)(intptr_t)tex,
                avail,
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            // Sahneye Sürükle ve Bırak (Drag & Drop: Model dosyasını sahneye bırakınca otomatik yerleştir ve materyalini bağla)
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                    auto* data = static_cast<const vesta::asset_drag_payload*>(payload->Data);
                    if (data && data->type == vesta::asset_payload_type::model && m_engine_ctx) {
                        std::string modelPathU8 = data->path;

                        // 1. Model için dokuları ve PBR materyali otomatik oluştur / tespit et
                        auto autoMats = aria::asset_loader::extract_and_generate_materials_for_model(modelPathU8, m_engine_ctx->aria_system);

                        // 2. Modeli Aria önbelleğine yükle
                        aria::handle<aria::model_asset> modelHandle;
                        if (m_engine_ctx->aria_system) {
                            modelHandle = asset_pipeline::import_model(m_engine_ctx->aria_system, modelPathU8);
                        }

                        const aria::model_asset* modelAsset = (m_engine_ctx->aria_system && modelHandle.is_valid())
                            ? m_engine_ctx->aria_system->get_model_data(modelHandle)
                            : nullptr;

                        scene& curScene = m_engine_ctx->session.editor_scene();
                        registry& reg = curScene.get_registry();

                        std::filesystem::path mp = qual::path_from_utf8(modelPathU8);
                        std::string rootName = qual::path_to_utf8(mp.stem());

                        // Kamera önünde spawn konumu belirle
                        glm::vec3 spawnPos(0.0f);
                        if (m_editor_ctx && m_editor_ctx->camera) {
                            glm::vec3 camPos = m_editor_ctx->camera->position;
                            glm::vec3 camFwd = calc_forward(m_editor_ctx->camera->rotation);
                            spawnPos = camPos + camFwd * 5.0f;
                        }

                        // 3. Materyal handle'larını hazırla
                        std::vector<aria::handle<aria::material_asset>> loadedMatHandles;
                        if (m_engine_ctx->aria_system) {
                            for (const auto& matInfo : autoMats) {
                                loadedMatHandles.push_back(m_engine_ctx->aria_system->import_material(matInfo.material_guid, matInfo.material_path));
                            }
                        }

                        const size_t submeshCount = modelAsset ? modelAsset->sub_meshes.size() : 0;

                        if (submeshCount > 1) {
                            // ========================================================
                            // ÇOKLU PARÇA (HIERARCHICAL PREFAB/MODEL) MODU
                            // Kök entity oluşturulur ve her alt ağ (sub-mesh) için çocuk entity eklenir
                            // ========================================================
                            entity rootEnt = curScene.create_entity();
                            reg.add_component<name_component>(rootEnt, name_component{ rootName });

                            transform_component rootTc;
                            rootTc.position = spawnPos;
                            reg.add_component<transform_component>(rootEnt, rootTc);
                            reg.add_component<hierarchy_component>(rootEnt, hierarchy_component{});

                            for (size_t i = 0; i < submeshCount; ++i) {
                                const auto& sm = modelAsset->sub_meshes[i];
                                entity childEnt = curScene.create_entity();

                                // Alt parça ismi
                                std::string childName = sm.name.empty() ? (rootName + "_Part_" + std::to_string(i)) : sm.name;
                                reg.add_component<name_component>(childEnt, name_component{ childName });

                                // Alt parçanın yerel dönüşümü (Assimp düğüm dönüşümünden çıkarılmış)
                                transform_component childTc;
                                ecs_transform::apply_world_matrix_as_local(childTc, sm.local_transform);
                                reg.add_component<transform_component>(childEnt, childTc);

                                // Alt ağ (Mesh) bileşeni
                                mesh_component childMc;
                                childMc.source = mesh_source_type::model;
                                childMc.model_handle = modelHandle;
                                childMc.sub_mesh_index = i;
                                reg.add_component<mesh_component>(childEnt, childMc);

                                // Sınır (Bounds) bileşeni
                                bounds_component childBc;
                                childBc.localCenter = sm.local_center;
                                childBc.localRadius = (sm.local_radius > 0.001f) ? sm.local_radius : 1.0f;
                                reg.add_component<bounds_component>(childEnt, childBc);

                                // Materyal eşleştirmesi
                                if (!loadedMatHandles.empty()) {
                                    material_component childMat;
                                    // Alt parçanın ait olduğu materyal indeksini (mMaterialIndex) bağla
                                    const size_t matIdx = (sm.material_index < loadedMatHandles.size()) ? sm.material_index : 0;
                                    childMat.material_handle = loadedMatHandles[matIdx];
                                    reg.add_component<material_component>(childEnt, childMat);
                                }

                                // Ebeveyne hiyerarşik olarak bağla
                                vesta::attach_child_to_parent(reg, childEnt, rootEnt);
                            }

                            m_engine_ctx->vestaSys.selection().select_entity(rootEnt, false);
                        }
                        else {
                            // ========================================================
                            // TEKİL PARÇA (SINGLE MESH) MODU
                            // Doğrudan ana entity üzerine mesh bileşeni eklenir
                            // ========================================================
                            entity newEnt = curScene.create_entity();
                            reg.add_component<name_component>(newEnt, name_component{ rootName });

                            transform_component tc;
                            tc.position = spawnPos;
                            if (modelAsset && !modelAsset->sub_meshes.empty()) {
                                ecs_transform::apply_world_matrix_as_local(tc, modelAsset->sub_meshes[0].local_transform);
                                tc.position = spawnPos; // Kök nesne için kamera önü konumu korunur
                            }
                            reg.add_component<transform_component>(newEnt, tc);

                            mesh_component mc;
                            mc.source = mesh_source_type::model;
                            mc.model_handle = modelHandle;
                            mc.sub_mesh_index = 0;
                            reg.add_component<mesh_component>(newEnt, mc);

                            bounds_component bc;
                            if (modelAsset && !modelAsset->sub_meshes.empty()) {
                                bc.localCenter = modelAsset->sub_meshes[0].local_center;
                                bc.localRadius = (modelAsset->sub_meshes[0].local_radius > 0.001f) ? modelAsset->sub_meshes[0].local_radius : 1.0f;
                            } else {
                                bc.localCenter = glm::vec3(0.0f);
                                bc.localRadius = 1.0f;
                            }
                            reg.add_component<bounds_component>(newEnt, bc);

                            if (!loadedMatHandles.empty()) {
                                material_component matComp;
                                matComp.material_handle = loadedMatHandles[0];
                                reg.add_component<material_component>(newEnt, matComp);
                            }

                            reg.add_component<hierarchy_component>(newEnt, hierarchy_component{});
                            m_engine_ctx->vestaSys.selection().select_entity(newEnt, false);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::GetWindowDrawList()->PopClipRect();
        }

        ImGui::PopStyleVar(2);

        // Etkileşimli Transform Gizmosu, Işık Tutamaçları, Billboard İkonlar ve Üst Araç Çubuğu
        if (m_engine_ctx && m_editor_ctx && avail.x > 0.0f && avail.y > 0.0f)
        {
            m_gizmo_overlay.render(
                *m_engine_ctx,
                *m_editor_ctx,
                scene_state.position,
                scene_state.size,
                scene_state.focused,
                scene_state.hovered
            );
        }

        ImGui::PopID();
        ImGui::End();
    }

} // namespace vesta
