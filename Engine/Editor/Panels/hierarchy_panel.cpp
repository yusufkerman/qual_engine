#include "hierarchy_panel.h"
#include "../../ECS/registry.h"
#include <imgui.h>
#include <algorithm> // std::find ve std::move için

static std::string getEntityName(registry& reg, entity e)
{
    if (auto* n = reg.get_component<name_component>(e))
        return n->name;

    return "Entity";
}

void hierarchy_panel::render(scene& scene,
    int& selectedEntity,
    int& renamingEntity,
    char* renameBuffer,
    size_t renameBufferSize,
    std::vector<entity>& deleteQueue,
    EditorContext& ctx)
{
    auto& reg = scene.get_registry();
    // Sahne içi sıralı entity listesini referans olarak alıyoruz
    auto& sceneEntities = const_cast<std::vector<entity>&>(scene.get_entities());

    ImGui::Begin("Hierarchy");

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        ctx.focusedPanel = FocusedPanel::Hierarchy;
    }

    // CREATE OBJECT
    if (ImGui::Button("Create Object"))
    {
        entity e = scene.create_entity();
        reg.add_component<transform_component>(e, transform_component{});
        reg.add_component<name_component>(e, name_component{ "New Entity" });

        selectedEntity = (int)e;
        renamingEntity = (int)e;
        strcpy_s(renameBuffer, renameBufferSize, "New Entity");
    }

    ImGui::Separator();

    // BOŞ ALANA SAĞ TIKLANDIĞINDA PASTE YAPABİLMEK İÇİN POPUP
    if (ImGui::BeginPopupContextWindow("Hierarchy_Window_Context", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Paste", "Ctrl+V", false, ctx.clipboardEntity != -1))
        {
            entity duplicated = reg.duplicate_entity((entity)ctx.clipboardEntity);
            sceneEntities.push_back(duplicated); // Sahne listesine ekle
            selectedEntity = (int)duplicated;
        }
        ImGui::EndPopup();
    }

    // KEYBOARD SHORTCUTS (Klavye Kısayolları)
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_C) && selectedEntity != -1)
            {
                ctx.clipboardEntity = selectedEntity;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_V) && ctx.clipboardEntity != -1)
            {
                entity duplicated = reg.duplicate_entity((entity)ctx.clipboardEntity);
                sceneEntities.push_back(duplicated);
                selectedEntity = (int)duplicated;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_D) && selectedEntity != -1)
            {
                entity duplicated = reg.duplicate_entity((entity)selectedEntity);
                sceneEntities.push_back(duplicated);
                selectedEntity = (int)duplicated;
            }
        }
    }

    // ENTITY LIST RENDERER
    for (size_t i = 0; i < sceneEntities.size(); ++i)
    {
        entity e = sceneEntities[i];
        std::string name = getEntityName(reg, e);
        bool isSelected = (selectedEntity == (int)e);

        ImGui::PushID((int)e);

        if (renamingEntity != (int)e)
        {
            // Selectable yapısını Drag & Drop ile uyumlu hale getirmek için leaf flag'leri eklenebilir
            if (ImGui::Selectable(name.c_str(), isSelected, ImGuiSelectableFlags_AllowOverlap))
                selectedEntity = (int)e;

            // DRAG AND DROP: SOURCE (Sürükleme Başlangıcı)
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                // Sürüklenen entity'nin indeksini (i) payload olarak gönderiyoruz
                ImGui::SetDragDropPayload("HIERARCHY_REORDER", &i, sizeof(size_t));
                ImGui::Text("Moving: %s", name.c_str());
                ImGui::EndDragDropSource();
            }

            // DRAG AND DROP: TARGET (Bırakılma Noktası)
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_REORDER"))
                {
                    size_t sourceIndex = *(const size_t*)payload->Data;
                    size_t targetIndex = i;

                    if (sourceIndex != targetIndex)
                    {
                        // Vektör içinde elemanın yerini kaydırıyoruz
                        entity movingEntity = sceneEntities[sourceIndex];
                        sceneEntities.erase(sceneEntities.begin() + sourceIndex);
                        sceneEntities.insert(sceneEntities.begin() + targetIndex, movingEntity);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // POPUP CONTEXT (Sağ Tık Menüsü)
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    renamingEntity = (int)e;
                    strcpy_s(renameBuffer, renameBufferSize, name.c_str());
                }

                if (ImGui::MenuItem("Copy", "Ctrl+C"))
                {
                    ctx.clipboardEntity = (int)e;
                }

                if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
                {
                    entity duplicated = reg.duplicate_entity(e);
                    // Profesyonel yaklaşım: Duplicate edilen nesneyi hemen altına ekle
                    sceneEntities.insert(sceneEntities.begin() + i + 1, duplicated);
                    selectedEntity = (int)duplicated;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Delete", "Del"))
                {
                    if (selectedEntity == (int)e)
                        selectedEntity = -1;

                    deleteQueue.push_back(e);
                }

                ImGui::EndPopup();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                renamingEntity = (int)e;
                strcpy_s(renameBuffer, renameBufferSize, name.c_str());
            }
        }
        else
        {
            // Rename input alanı (Mevcut kodun aynen korunuyor)
            ImGui::SetNextItemWidth(160);
            bool enter = ImGui::InputText("##rename", renameBuffer, renameBufferSize, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            bool cancel = ImGui::SmallButton("X");
            bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape);

            if (enter || cancel || esc)
            {
                if (enter)
                {
                    if (auto* n = reg.get_component<name_component>(e))
                        n->name = renameBuffer;
                    else
                        reg.add_component<name_component>(e, name_component{ renameBuffer });
                }
                renamingEntity = -1;
            }
        }

        ImGui::PopID();
    }

    ImGui::End();
}