/**
 * @file inspector_panel.cpp
 * @brief Inspector panelinin ImGui ile bileşen, varlık ve Unity tarzı nesne ikonu çizim işlemlerini barındırır.
 */

#include "inspector_panel.h"
#include "scene_gizmo_overlay.h"
#include "../../../ECS/components.h"
#include "../../../ECS/registry.h"
#include <imgui.h>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace vesta
{
    inspector_panel::inspector_panel(std::unique_ptr<inspector_view_model> vm, gizmo_settings* gizmoSettings)
        : m_vm(std::move(vm))
        , m_gizmo_settings(gizmoSettings)
    {
    }

    inspector_panel::~inspector_panel() = default;

    void inspector_panel::render()
    {
        if (!m_is_open)
            return;

        m_vm->update();

        ImGui::Begin(m_title.c_str(), &m_is_open);

        switch (m_vm->get_mode())
        {
            // ========================================================
            // 1. SEÇİLİ ÖĞE YOK (NOTHING SELECTED)
            // ========================================================
        case inspector_mode::none:
        {
            ImGui::TextDisabled("Nothing selected.");
            break;
        }

        // ========================================================
        // 2. SAHNE NESNESİ DENETİMİ (ENTITY INSPECTOR)
        // ========================================================
        case inspector_mode::entity:
        {
            const auto& selectedEntities = m_vm->get_selected_entities();

            // ----------------------------------------------------
            // ÇOKLU SEÇİM MODU (MULTI-SELECTION INSPECTOR)
            // ----------------------------------------------------
            if (selectedEntities.size() > 1)
            {
                ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Multi-Selection: %zu Entities", selectedEntities.size());
                ImGui::Separator();

                // Çoklu İkon Atama
                if (m_gizmo_settings)
                {
                    if (ImGui::Button("Icon (Set for All)"))
                    {
                        ImGui::OpenPopup("MultiInspectorEntityIconPopup");
                    }
                    if (ImGui::BeginPopup("MultiInspectorEntityIconPopup"))
                    {
                        ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Select Entity Scene/Hierarchy Icon for All");
                        ImGui::Separator();
                        for (int i = 0; i <= static_cast<int>(entity_icon_type::label_purple); ++i)
                        {
                            const auto candidate = static_cast<entity_icon_type>(i);
                            if (ImGui::MenuItem(scene_gizmo_overlay::get_icon_type_name(candidate)))
                            {
                                for (entity e : selectedEntities)
                                {
                                    m_gizmo_settings->set_entity_icon(e, candidate);
                                }
                            }
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::SameLine();
                }

                // Çoklu Bileşen Ekleme
                if (ImGui::Button("Add Component to All"))
                {
                    ImGui::OpenPopup("MultiAddComponentPopup");
                }
                if (ImGui::BeginPopup("MultiAddComponentPopup"))
                {
                    static char multiSearchBuf[64] = "";
                    ImGui::SetNextItemWidth(220.0f);
                    if (ImGui::IsWindowAppearing())
                    {
                        multiSearchBuf[0] = '\0';
                        ImGui::SetKeyboardFocusHere();
                    }
                    ImGui::InputTextWithHint("##MultiCompSearch", "Search Component...", multiSearchBuf, sizeof(multiSearchBuf));
                    ImGui::Separator();

                    std::string filterStr = multiSearchBuf;
                    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

                    const auto& addableList = m_vm->get_addable_components();

                    if (!filterStr.empty())
                    {
                        // Arama modunda düz eşleşenler
                        int matchCount = 0;
                        for (const auto& compEntry : addableList)
                        {
                            std::string lowerName = compEntry.info->display_name;
                            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                            if (lowerName.find(filterStr) != std::string::npos)
                            {
                                matchCount++;
                                if (ImGui::MenuItem(compEntry.info->display_name.c_str()))
                                {
                                    m_vm->request_add_component_to_all(compEntry.hash);
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        }
                        if (matchCount == 0)
                        {
                            ImGui::TextDisabled("No components match.");
                        }
                    }
                    else
                    {
                        // Kategorili görünüm
                        std::map<std::string, std::vector<const add_component_entry*>> categorized;
                        for (const auto& compEntry : addableList)
                        {
                            std::string cat = compEntry.info->category.empty() ? "General" : compEntry.info->category;
                            categorized[cat].push_back(&compEntry);
                        }

                        for (const auto& [catName, entries] : categorized)
                        {
                            if (ImGui::BeginMenu(catName.c_str()))
                            {
                                for (const auto* compEntry : entries)
                                {
                                    if (ImGui::MenuItem(compEntry->info->display_name.c_str()))
                                    {
                                        m_vm->request_add_component_to_all(compEntry->hash);
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                                ImGui::EndMenu();
                            }
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::Separator();

                auto& reg = m_vm->get_registry();

                // 1. TRANSFORM ÇOKLU DÜZENLEME
                std::vector<entity> transformEntities;
                glm::vec3 avgPos(0.0f);
                glm::vec3 avgRot(0.0f);
                glm::vec3 avgScale(0.0f);
                for (entity e : selectedEntities)
                {
                    auto* t = reg.get_component<transform_component>(e);
                    if (t)
                    {
                        transformEntities.push_back(e);
                        avgPos += t->position;
                        avgRot += t->rotation;
                        avgScale += t->scale;
                    }
                }

                if (!transformEntities.empty())
                {
                    const float invCount = 1.0f / static_cast<float>(transformEntities.size());
                    avgPos *= invCount;
                    avgRot *= invCount;
                    avgScale *= invCount;

                    if (ImGui::CollapsingHeader("Transform (Multi-Selection)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        glm::vec3 posVal = avgPos;
                        if (ImGui::DragFloat3("Position (Delta)", &posVal.x, 0.1f))
                        {
                            const glm::vec3 delta = posVal - avgPos;
                            for (entity e : transformEntities)
                            {
                                if (auto* t = reg.get_component<transform_component>(e))
                                {
                                    t->position += delta;
                                }
                            }
                        }

                        glm::vec3 rotVal = avgRot;
                        if (ImGui::DragFloat3("Rotation (Delta)", &rotVal.x, 0.1f))
                        {
                            const glm::vec3 delta = rotVal - avgRot;
                            for (entity e : transformEntities)
                            {
                                if (auto* t = reg.get_component<transform_component>(e))
                                {
                                    t->rotation += delta;
                                }
                            }
                        }

                        glm::vec3 scaleVal = avgScale;
                        if (ImGui::DragFloat3("Scale (Delta)", &scaleVal.x, 0.1f))
                        {
                            const glm::vec3 delta = scaleVal - avgScale;
                            for (entity e : transformEntities)
                            {
                                if (auto* t = reg.get_component<transform_component>(e))
                                {
                                    t->scale += delta;
                                    t->scale = glm::max(t->scale, glm::vec3(0.001f));
                                }
                            }
                        }

                        if (ImGui::SmallButton("Set All Scales to 1.0"))
                        {
                            for (entity e : transformEntities)
                            {
                                if (auto* t = reg.get_component<transform_component>(e))
                                {
                                    t->scale = glm::vec3(1.0f);
                                }
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Align Positions to First"))
                        {
                            auto* firstT = reg.get_component<transform_component>(transformEntities.front());
                            if (firstT)
                            {
                                for (entity e : transformEntities)
                                {
                                    if (auto* t = reg.get_component<transform_component>(e))
                                    {
                                        t->position = firstT->position;
                                    }
                                }
                            }
                        }
                    }
                }

                // 2. LIGHT ÇOKLU DÜZENLEME
                std::vector<entity> lightEntities;
                for (entity e : selectedEntities)
                {
                    if (reg.has_component<light_component>(e))
                        lightEntities.push_back(e);
                }
                if (!lightEntities.empty())
                {
                    if (ImGui::CollapsingHeader("Light (Multi-Selection)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        auto* refLight = reg.get_component<light_component>(lightEntities.front());
                        glm::vec3 col = refLight->color;
                        if (ImGui::ColorEdit3("Color", &col.x))
                        {
                            for (entity e : lightEntities)
                            {
                                if (auto* l = reg.get_component<light_component>(e)) l->color = col;
                            }
                        }

                        float intensity = refLight->intensity;
                        if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
                        {
                            for (entity e : lightEntities)
                            {
                                if (auto* l = reg.get_component<light_component>(e)) l->intensity = intensity;
                            }
                        }

                        float range = refLight->range;
                        if (ImGui::DragFloat("Range", &range, 0.5f, 0.1f, 500.0f))
                        {
                            for (entity e : lightEntities)
                            {
                                if (auto* l = reg.get_component<light_component>(e)) l->range = range;
                            }
                        }

                        const char* typeNames[] = { "Directional", "Point", "Spot" };
                        int typeIndex = static_cast<int>(refLight->type);
                        if (ImGui::Combo("Type", &typeIndex, typeNames, 3))
                        {
                            for (entity e : lightEntities)
                            {
                                if (auto* l = reg.get_component<light_component>(e))
                                    l->type = static_cast<light_type>(typeIndex);
                            }
                        }

                        if (refLight->type == light_type::spot)
                        {
                            float inCone = refLight->inner_cone;
                            if (ImGui::DragFloat("Inner Cone", &inCone, 0.5f, 0.0f, 89.0f))
                            {
                                for (entity e : lightEntities)
                                {
                                    if (auto* l = reg.get_component<light_component>(e)) l->inner_cone = inCone;
                                }
                            }
                            float outCone = refLight->outer_cone;
                            if (ImGui::DragFloat("Outer Cone", &outCone, 0.5f, 0.0f, 90.0f))
                            {
                                for (entity e : lightEntities)
                                {
                                    if (auto* l = reg.get_component<light_component>(e)) l->outer_cone = outCone;
                                }
                            }
                        }

                        bool castShadows = refLight->cast_shadows;
                        if (ImGui::Checkbox("Cast Shadows", &castShadows))
                        {
                            for (entity e : lightEntities)
                            {
                                if (auto* l = reg.get_component<light_component>(e)) l->cast_shadows = castShadows;
                            }
                        }
                    }
                }

                // 3. COLLIDER ÇOKLU DÜZENLEME
                std::vector<entity> colliderEntities;
                for (entity e : selectedEntities)
                {
                    if (reg.has_component<collider_component>(e))
                        colliderEntities.push_back(e);
                }
                if (!colliderEntities.empty())
                {
                    if (ImGui::CollapsingHeader("Collider (Multi-Selection)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        auto* refCol = reg.get_component<collider_component>(colliderEntities.front());
                        const char* colliderNames[] = { "Box", "Sphere" };
                        int typeIndex = static_cast<int>(refCol->type);
                        if (ImGui::Combo("Collider Type", &typeIndex, colliderNames, 2))
                        {
                            for (entity e : colliderEntities)
                            {
                                if (auto* c = reg.get_component<collider_component>(e))
                                {
                                    c->type = static_cast<collider_type>(typeIndex);
                                    c->extents = glm::vec3(0.5f);
                                }
                            }
                        }

                        if (refCol->type == collider_type::box)
                        {
                            glm::vec3 displayedSize = refCol->extents * 2.0f;
                            if (ImGui::DragFloat3("Box Size (Full)", &displayedSize.x, 0.1f, 0.01f, 100.0f, "%.2f"))
                            {
                                for (entity e : colliderEntities)
                                {
                                    if (auto* c = reg.get_component<collider_component>(e))
                                        c->extents = displayedSize * 0.5f;
                                }
                            }
                        }
                        else
                        {
                            float radius = refCol->extents.x;
                            if (ImGui::DragFloat("Radius", &radius, 0.05f, 0.01f, 100.0f, "%.2f"))
                            {
                                for (entity e : colliderEntities)
                                {
                                    if (auto* c = reg.get_component<collider_component>(e))
                                        c->extents = glm::vec3(radius);
                                }
                            }
                        }
                    }
                }

                // 4. RIGIDBODY ÇOKLU DÜZENLEME
                std::vector<entity> rbEntities;
                for (entity e : selectedEntities)
                {
                    if (reg.has_component<rigidbody_component>(e))
                        rbEntities.push_back(e);
                }
                if (!rbEntities.empty())
                {
                    if (ImGui::CollapsingHeader("Rigidbody (Multi-Selection)", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        auto* refRb = reg.get_component<rigidbody_component>(rbEntities.front());
                        const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };
                        int motionIndex = static_cast<int>(refRb->motion);
                        if (ImGui::Combo("Motion Type", &motionIndex, motionNames, 3))
                        {
                            for (entity e : rbEntities)
                            {
                                if (auto* rb = reg.get_component<rigidbody_component>(e))
                                    rb->motion = static_cast<motion_type>(motionIndex);
                            }
                        }

                        float mass = refRb->mass;
                        if (ImGui::DragFloat("Mass (Kg)", &mass, 0.05f, 0.01f, 10000.0f, "%.2f"))
                        {
                            for (entity e : rbEntities)
                            {
                                if (auto* rb = reg.get_component<rigidbody_component>(e))
                                    rb->mass = mass;
                            }
                        }

                        float friction = refRb->friction;
                        if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f, "%.2f"))
                        {
                            for (entity e : rbEntities)
                            {
                                if (auto* rb = reg.get_component<rigidbody_component>(e))
                                    rb->friction = friction;
                            }
                        }

                        float restitution = refRb->restitution;
                        if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f, "%.2f"))
                        {
                            for (entity e : rbEntities)
                            {
                                if (auto* rb = reg.get_component<rigidbody_component>(e))
                                    rb->restitution = restitution;
                            }
                        }
                    }
                }

                // 5. SEÇİLİ NESNELER LİSTESİ
                if (ImGui::CollapsingHeader("Selected Entities List"))
                {
                    for (entity e : selectedEntities)
                    {
                        std::string name = "Entity " + std::to_string(e);
                        if (auto* nc = reg.get_component<name_component>(e))
                        {
                            if (!nc->name.empty()) name = nc->name;
                        }
                        ImGui::BulletText("%s (ID: %u)", name.c_str(), static_cast<uint32_t>(e));
                    }
                }

                break;
            }

            // ----------------------------------------------------
            // TEKİL SEÇİM MODU (SINGLE ENTITY INSPECTOR)
            // ----------------------------------------------------
            const entity selectedEnt = m_vm->get_selected_entity();

            if (ImGui::Button("Add Component"))
            {
                ImGui::OpenPopup("AddComponentPopup");
            }

            // Unity tarzı Sahne/Hiyerarşi İkon Seçici
            if (m_gizmo_settings && selectedEnt != 0)
            {
                ImGui::SameLine();
                if (ImGui::Button("Icon (Gizmo)"))
                {
                    ImGui::OpenPopup("InspectorEntityIconPopup");
                }

                const entity_icon_type resolved = scene_gizmo_overlay::resolve_entity_icon(
                    selectedEnt, m_vm->get_registry(), *m_gizmo_settings);
                if (resolved != entity_icon_type::none && resolved != entity_icon_type::auto_detect)
                {
                    ImGui::SameLine();
                    const ImVec2 p = ImGui::GetCursorScreenPos();
                    ImGui::Dummy(ImVec2(20.0f, ImGui::GetFrameHeight()));
                    scene_gizmo_overlay::draw_icon_badge_2d(
                        resolved,
                        glm::vec2(p.x + 10.0f, p.y + ImGui::GetFrameHeight() * 0.5f),
                        8.5f);
                }

                if (ImGui::BeginPopup("InspectorEntityIconPopup"))
                {
                    ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Select Entity Scene/Hierarchy Icon");
                    ImGui::Separator();

                    const entity_icon_type currentAssigned = m_gizmo_settings->get_entity_icon(selectedEnt);
                    for (int i = 0; i <= static_cast<int>(entity_icon_type::label_purple); ++i)
                    {
                        const auto candidate = static_cast<entity_icon_type>(i);
                        if (ImGui::MenuItem(scene_gizmo_overlay::get_icon_type_name(candidate), nullptr, currentAssigned == candidate))
                        {
                            m_gizmo_settings->set_entity_icon(selectedEnt, candidate);
                        }
                    }
                    ImGui::EndPopup();
                }
            }

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                static char singleSearchBuf[64] = "";
                ImGui::SetNextItemWidth(220.0f);
                if (ImGui::IsWindowAppearing())
                {
                    singleSearchBuf[0] = '\0';
                    ImGui::SetKeyboardFocusHere();
                }
                ImGui::InputTextWithHint("##SingleCompSearch", "Search Component...", singleSearchBuf, sizeof(singleSearchBuf));
                ImGui::Separator();

                std::string filterStr = singleSearchBuf;
                std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

                const auto& addableList = m_vm->get_addable_components();

                if (!filterStr.empty())
                {
                    int matchCount = 0;
                    for (const auto& component : addableList)
                    {
                        std::string lowerName = component.info->display_name;
                        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                        if (lowerName.find(filterStr) != std::string::npos)
                        {
                            matchCount++;
                            if (ImGui::MenuItem(component.info->display_name.c_str()))
                            {
                                m_vm->request_add_component(component.hash);
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                    if (matchCount == 0)
                    {
                        ImGui::TextDisabled("No components match.");
                    }
                }
                else
                {
                    std::map<std::string, std::vector<const add_component_entry*>> categorized;
                    for (const auto& component : addableList)
                    {
                        std::string cat = component.info->category.empty() ? "General" : component.info->category;
                        categorized[cat].push_back(&component);
                    }

                    for (const auto& [catName, entries] : categorized)
                    {
                        if (ImGui::BeginMenu(catName.c_str()))
                        {
                            for (const auto* component : entries)
                            {
                                if (ImGui::MenuItem(component->info->display_name.c_str()))
                                {
                                    m_vm->request_add_component(component->hash);
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            for (const auto& component : m_vm->get_components())
            {
                ImGui::PushID(static_cast<int>(component.hash));

                if (component.removable)
                {
                    ImGui::SetNextItemAllowOverlap();
                }

                ImGuiTreeNodeFlags headerFlags = component.default_open ? ImGuiTreeNodeFlags_DefaultOpen : 0;
                if (component.removable)
                {
                    headerFlags |= ImGuiTreeNodeFlags_AllowOverlap;
                }

                // Başlık çubuğu ve açılır/kapanır sekme kontrolü
                bool opened = ImGui::CollapsingHeader(
                    component.display_name.c_str(),
                    headerFlags
                );

                if (component.removable)
                {
                    // Sağ tık menüsü (Unity tarzı alternatif kaldırma seçeneği)
                    if (ImGui::BeginPopupContextItem("ComponentHeaderContextMenu"))
                    {
                        if (ImGui::MenuItem("Remove Component"))
                        {
                            m_vm->request_remove_component(component.hash);
                            ImGui::EndPopup();
                            ImGui::PopID();
                            continue;
                        }
                        ImGui::EndPopup();
                    }

                    // Silme butonunu başlığın sağ kenarına hizala ve üst üste binmede öncelik ver
                    const float btnWidth = 58.0f;
                    const float btnX = ImGui::GetWindowContentRegionMax().x - btnWidth - 4.0f;
                    ImGui::SameLine(btnX);

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.22f, 0.22f, 0.75f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.15f, 0.15f, 1.0f));

                    if (ImGui::SmallButton("Remove"))
                    {
                        m_vm->request_remove_component(component.hash);
                        ImGui::PopStyleColor(3);
                        ImGui::PopID();
                        continue; // Silinen bileşenin kalan arayüzünü bu kare çizmeden atla
                    }
                    ImGui::PopStyleColor(3);
                }

                if (opened)
                {
                    if (component.ui && component.ui->render_fn)
                    {
                        component.ui->render_fn(
                            m_vm->get_selected_entity(),
                            m_vm->get_registry()
                        );
                    }
                }

                ImGui::PopID();
            }

            break;
        }

        // ========================================================
        // 3. DOSYA/VARLIK DENETİMİ (ASSET INSPECTOR)
        // ========================================================
        case inspector_mode::asset:
        {
            auto asset = m_vm->get_asset_data();

            if (!asset.ui || !asset.ui->render_fn)
            {
                ImGui::TextDisabled("No inspector registered for this asset type.");
                break;
            }

            asset.ui->render_fn(asset.asset_path);
            break;
        }
        }

        ImGui::End();
    }

} // namespace vesta