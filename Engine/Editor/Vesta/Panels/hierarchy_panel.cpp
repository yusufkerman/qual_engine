/**
 * @file hierarchy_panel.cpp
 * @brief Sahne hiyerarşisi panelinin ImGui çizim, Unity tarzı ikon rozeti/ataması ve kısayol dinleme işlemlerini barındırır.
 */

#include "hierarchy_panel.h"
#include "scene_gizmo_overlay.h"
#include "../../editor_camera.h"
#include "../../editor_context.h"
#include <iostream>
#include <algorithm>
#include "../../../ECS/transform_utils.h"
#include "../Inspector/Components/ui_components_ui.h"
#include <imgui.h>
#include <cstdio> // snprintf için

namespace vesta {

    hierarchy_panel::hierarchy_panel(
        IEditableWorld& world,
        registry& reg,
        editor_selection_manager& selection,
        gizmo_settings* gizmoSettings,
        EditorCamera* camera,
        EditorContext* editorCtx)
        : m_vm(std::make_unique<hierarchy_view_model>(world, reg, selection))
        , m_gizmo_settings(gizmoSettings)
        , m_camera(camera)
        , m_editor_ctx(editorCtx)
    {
    }

    EditorCamera* hierarchy_panel::get_camera() const noexcept
    {
        if (m_editor_ctx && m_editor_ctx->camera)
            return m_editor_ctx->camera;
        return m_camera;
    }

    /**
     * @brief Ağaç yapısındaki tekil bir varlığı (Entity) ve Unity tarzı ikon rozetini çizer.
     */
    static void draw_entity_node(
        hierarchy_view_model* vm,
        const hierarchy_node_data& node,
        gizmo_settings* gizmoSettings,
        EditorCamera* camera)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!node.has_children) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (node.is_selected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool opened = false;

        // ==========================================
        // YENİDEN ADLANDIRMA (RENAME) MODU
        // ==========================================
        if (vm->is_renaming(node.id))
        {
            static char rename_buffer[256];
            static entity editing_entity = 0;

            if (editing_entity != node.id)
            {
                editing_entity = node.id;
                snprintf(rename_buffer, sizeof(rename_buffer), "%s", vm->get_rename_buffer().c_str());
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::SetNextItemWidth(-1.0f);

            bool commit = ImGui::InputText(
                "##Rename",
                rename_buffer,
                sizeof(rename_buffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll
            );

            if (commit)
            {
                vm->get_rename_buffer() = rename_buffer;
                vm->commit_rename();
                editing_entity = 0;
            }

            if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                vm->cancel_rename();
                editing_entity = 0;
            }

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                vm->get_rename_buffer() = rename_buffer;
                vm->commit_rename();
                editing_entity = 0;
            }

            return; // Düzenleme modundayken alt hiyerarşiyi çizmeye devam etme
        }
        else
        {
            // İkon rozeti aktifse ağaç düğümünün sağında veya solunda rozet çiz
            entity_icon_type resolvedIcon = entity_icon_type::none;
            glm::vec3 iconTint(1.0f);
            if (gizmoSettings && gizmoSettings->show_hierarchy_badges)
            {
                resolvedIcon = scene_gizmo_overlay::resolve_entity_icon(node.id, vm->get_registry(), *gizmoSettings);
                if (const auto* lc = vm->get_registry().get_component<light_component>(node.id))
                {
                    iconTint = lc->color;
                }
            }

            if (resolvedIcon != entity_icon_type::none && resolvedIcon != entity_icon_type::auto_detect)
            {
                const ImVec2 cursorBefore = ImGui::GetCursorScreenPos();
                opened = ImGui::TreeNodeEx(
                    reinterpret_cast<void*>(static_cast<uintptr_t>(node.id)),
                    flags,
                    "     %s",
                    node.name.c_str());

                const float lineH = ImGui::GetFrameHeight();
                const glm::vec2 badgeCenter(cursorBefore.x + 28.0f, cursorBefore.y + lineH * 0.48f);
                scene_gizmo_overlay::draw_icon_badge_2d(resolvedIcon, badgeCenter, 7.2f, iconTint);
            }
            else
            {
                opened = ImGui::TreeNodeEx(
                    reinterpret_cast<void*>(static_cast<uintptr_t>(node.id)),
                    flags,
                    "%s",
                    node.name.c_str());
            }

            // ==========================================
            // ÇİFT TIKLA ODAKLAN (FOCUS) - Unity / Unreal Stili
            // ==========================================
            const bool nodeHovered = ImGui::IsItemHovered();
            const bool nodeDoubleClicked = nodeHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

            if (nodeDoubleClicked && camera)
            {
                registry& reg = vm->get_registry();
                if (reg.has_component<transform_component>(node.id))
                {
                    const glm::mat4 worldMat = ecs_transform::get_world_matrix(node.id, reg);
                    const glm::vec3 worldPos = glm::vec3(worldMat[3]);
                    float focusDist = 5.0f;

                    if (reg.has_component<bounds_component>(node.id))
                    {
                        const auto* b = reg.get_component<bounds_component>(node.id);
                        focusDist = std::max(2.0f, b->localRadius * 2.5f);
                    }
                    else if (reg.has_component<collider_component>(node.id))
                    {
                        const auto* c = reg.get_component<collider_component>(node.id);
                        focusDist = std::max(2.0f, glm::length(c->extents) * 2.5f);
                    }

                    std::cout << "[Hierarchy] Focus on entity: '" << node.name << "' (ID: " << node.id
                              << ") at (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z
                              << ") dist: " << focusDist << std::endl;
                    camera->focus_on(worldPos, focusDist, false);
                }
            }
        }

        // ==========================================
        // ==========================================
        // SEÇİM (SELECTION)
        // ==========================================
        if (ImGui::IsItemClicked()) {
            const bool multiKey = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;
            if (multiKey) {
                vm->request_select(node.id, true);
            } else {
                // Önceden seçiliyse mouse down anında seçimi silme ki çoklu sürükleme (drag) yapılabilsin
                if (!vm->is_selected(node.id)) {
                    vm->request_select(node.id, false);
                }
            }
        }

        // Çoklu seçim yapılmış bir öğeye tıklanıp sürükleme yapılmadan bırakıldıysa tekil seçime dön
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
            !ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift) {
            if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f) && vm->is_selected(node.id)) {
                vm->request_select(node.id, false);
            }
        }

        // ==========================================
        // SÜRÜKLEME KAYNAĞI (DRAG SOURCE)
        // ==========================================
        if (ImGui::BeginDragDropSource()) {
            std::vector<entity> dragList;
            if (vm->is_selected(node.id)) {
                for (entity e : vm->get_selected_entities()) {
                    if (e != 0) dragList.push_back(e);
                }
            }
            if (dragList.empty()) {
                dragList.push_back(node.id);
            }

            ImGui::SetDragDropPayload("VESTA_ENTITY", dragList.data(), dragList.size() * sizeof(entity));
            if (dragList.size() == 1) {
                ImGui::Text("%s", node.name.c_str());
            } else {
                ImGui::Text("%s (+%d objects)", node.name.c_str(), static_cast<int>(dragList.size() - 1));
            }
            ImGui::EndDragDropSource();
        }

        // ==========================================
        // BIRAKMA HEDEFİ (DROP TARGET)
        // ==========================================
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VESTA_ENTITY")) {
                const size_t count = payload->DataSize / sizeof(entity);
                const entity* draggedEntities = static_cast<const entity*>(payload->Data);
                std::vector<entity> toReparent;
                toReparent.reserve(count);
                for (size_t i = 0; i < count; ++i) {
                    entity dragged = draggedEntities[i];
                    if (dragged != 0 && dragged != node.id) {
                        toReparent.push_back(dragged);
                    }
                }
                vm->request_set_parent_batch(toReparent, node.id);
            }
            ImGui::EndDragDropTarget();
        }



        // ==========================================
        // İÇERİK MENÜSÜ (CONTEXT MENU) & UNITY TARZI İKON SEÇİMİ
        // ==========================================
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename")) {
                vm->start_rename(node.id);
            }

            if (gizmoSettings && ImGui::BeginMenu("Set Icon (Gizmo)"))
            {
                const entity_icon_type currentAssigned = gizmoSettings->get_entity_icon(node.id);
                for (int i = 0; i <= static_cast<int>(entity_icon_type::label_purple); ++i)
                {
                    const auto candidate = static_cast<entity_icon_type>(i);
                    const bool isCurrent = (currentAssigned == candidate);
                    if (ImGui::MenuItem(scene_gizmo_overlay::get_icon_type_name(candidate), nullptr, isCurrent))
                    {
                        gizmoSettings->set_entity_icon(node.id, candidate);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();
            if (vm->can_undo() && ImGui::MenuItem("Undo", "Ctrl+Z")) {
                vm->request_undo();
            }
            if (vm->can_redo() && ImGui::MenuItem("Redo", "Ctrl+Y")) {
                vm->request_redo();
            }
            if (vm->can_undo() || vm->can_redo()) {
                ImGui::Separator();
            }

            if (ImGui::MenuItem("Copy")) {
                vm->request_copy(node.id);
            }
            if (ImGui::MenuItem("Duplicate")) {
                vm->request_duplicate(node.id);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                vm->request_destroy_entity(node.id);
            }
            ImGui::EndPopup();
        }

        // ==========================================
        // ALT DÜĞÜMLERİN ÇİZİMİ (CHILDREN)
        // ==========================================
        if (opened) {
            for (entity child : node.children) {
                draw_entity_node(vm, vm->get_node_data(child), gizmoSettings, camera);
            }
            ImGui::TreePop();
        }
    }

    void hierarchy_panel::render()
    {
        if (!m_is_open) return;

        ImGui::Begin(m_title.c_str(), &m_is_open);

        // ==========================================
        // KISAYOLLAR (SHORTCUTS)
        // ==========================================
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            bool ctrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);

            bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

            if (ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_Z)) {
                m_vm->request_undo();
            }
            else if ((ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) || (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_Z))) {
                m_vm->request_redo();
            }

            if (ctrl) {
                if (ImGui::IsKeyPressed(ImGuiKey_C)) m_vm->request_copy_selected();
                if (ImGui::IsKeyPressed(ImGuiKey_D)) m_vm->request_duplicate_selected();
                if (ImGui::IsKeyPressed(ImGuiKey_V)) m_vm->request_paste();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                m_vm->request_delete_selected();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                entity selected = m_vm->get_selected_entity();
                if (selected != 0) {
                    EditorCamera* cam = get_camera();
                    if (cam) {
                        registry& reg = m_vm->get_registry();
                        if (reg.has_component<transform_component>(selected)) {
                            const glm::mat4 worldMat = ecs_transform::get_world_matrix(selected, reg);
                            const glm::vec3 worldPos = glm::vec3(worldMat[3]);
                            float focusDist = 5.0f;
                            if (reg.has_component<bounds_component>(selected)) {
                                const auto* b = reg.get_component<bounds_component>(selected);
                                focusDist = std::max(2.0f, b->localRadius * 2.5f);
                            } else if (reg.has_component<collider_component>(selected)) {
                                const auto* c = reg.get_component<collider_component>(selected);
                                focusDist = std::max(2.0f, glm::length(c->extents) * 2.5f);
                            }
                            cam->focus_on(worldPos, focusDist, false);
                        }
                    }
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                entity selected = m_vm->get_selected_entity();
                if (selected != 0) {
                    EditorCamera* cam = get_camera();
                    if (cam) {
                        registry& reg = m_vm->get_registry();
                        if (reg.has_component<transform_component>(selected)) {
                            const glm::mat4 worldMat = ecs_transform::get_world_matrix(selected, reg);
                            const glm::vec3 worldPos = glm::vec3(worldMat[3]);
                            float focusDist = 5.0f;
                            if (reg.has_component<bounds_component>(selected)) {
                                const auto* b = reg.get_component<bounds_component>(selected);
                                focusDist = std::max(2.0f, b->localRadius * 2.5f);
                            } else if (reg.has_component<collider_component>(selected)) {
                                const auto* c = reg.get_component<collider_component>(selected);
                                focusDist = std::max(2.0f, glm::length(c->extents) * 2.5f);
                            }
                            cam->focus_on(worldPos, focusDist, false);
                        }
                    }
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                entity selected = m_vm->get_selected_entity();
                if (selected != 0) {
                    m_vm->start_rename(selected);
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                if (m_vm->get_renaming_entity() != 0) {
                    m_vm->cancel_rename();
                }
            }
        }

        // ==========================================
        // ÜST ARAÇ ÇUBUĞU (TOOLBAR)
        // ==========================================
        if (ImGui::Button("Create Object")) {
            m_vm->request_create_entity();
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Preset")) {
            ImGui::OpenPopup("HierarchyCreatePresetPopup");
        }

        if (ImGui::BeginPopup("HierarchyCreatePresetPopup"))
        {
            if (ImGui::MenuItem("Empty Entity")) {
                m_vm->request_create_entity("Empty Entity");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Camera")) {
                m_vm->request_create_camera();
            }
            if (ImGui::MenuItem("Point Light")) {
                m_vm->request_create_light(light_type::point);
            }
            if (ImGui::MenuItem("Spot Light")) {
                m_vm->request_create_light(light_type::spot);
            }
            if (ImGui::MenuItem("Directional Light")) {
                m_vm->request_create_light(light_type::directional);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("UI"))
            {
                if (ImGui::MenuItem("Canvas"))
                {
                    registry& reg = m_vm->get_registry();
                    entity canvas = ensure_canvas_exists(reg);
                    m_vm->request_select(canvas, false);
                }
                if (ImGui::MenuItem("Text"))
                {
                    registry& reg = m_vm->get_registry();
                    entity e = reg.create_entity();
                    reg.add_component<name_component>(e, name_component{ "Text" });
                    reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(160.0f, 40.0f) });
                    reg.add_component<ui_text_component>(e, ui_text_component{ "New Text" });
                    prepare_ui_entity(e, reg, false);
                    m_vm->request_select(e, false);
                }
                if (ImGui::MenuItem("Image"))
                {
                    registry& reg = m_vm->get_registry();
                    entity e = reg.create_entity();
                    reg.add_component<name_component>(e, name_component{ "Image" });
                    reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(100.0f, 100.0f) });
                    reg.add_component<ui_image_component>(e, ui_image_component{});
                    prepare_ui_entity(e, reg, false);
                    m_vm->request_select(e, false);
                }
                if (ImGui::MenuItem("Button"))
                {
                    registry& reg = m_vm->get_registry();
                    entity e = reg.create_entity();
                    reg.add_component<name_component>(e, name_component{ "Button" });
                    reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(160.0f, 48.0f) });
                    reg.add_component<ui_image_component>(e, ui_image_component{});
                    reg.add_component<ui_button_component>(e, ui_button_component{});
                    prepare_ui_entity(e, reg, false);

                    // Alt çocuk (Child) Text nesnesi
                    entity textChild = reg.create_entity();
                    reg.add_component<name_component>(textChild, name_component{ "Text" });
                    reg.add_component<rect_transform_component>(textChild, rect_transform_component{
                        glm::vec2(0.0f), glm::vec2(160.0f, 48.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.5f, 0.5f)
                    });
                    reg.add_component<ui_text_component>(textChild, ui_text_component{
                        "Button", "EngineResources/Fonts/Roboto-Regular.ttf", 20.0f, glm::vec4(0.12f, 0.12f, 0.12f, 1.0f)
                    });
                    attach_child_to_parent(reg, textChild, e);

                    m_vm->request_select(e, false);
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        // ==========================================
        // BOŞ ALAN İÇERİK MENÜSÜ (EMPTY AREA CONTEXT)
        // ==========================================
        if (ImGui::BeginPopupContextWindow("Hierarchy_Context", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (m_vm->can_undo() && ImGui::MenuItem("Undo", "Ctrl+Z")) {
                m_vm->request_undo();
            }
            if (m_vm->can_redo() && ImGui::MenuItem("Redo", "Ctrl+Y")) {
                m_vm->request_redo();
            }
            if (m_vm->can_undo() || m_vm->can_redo()) {
                ImGui::Separator();
            }

            if (ImGui::MenuItem("Create Empty")) {
                m_vm->request_create_entity();
            }
            if (ImGui::MenuItem("Create Camera")) {
                m_vm->request_create_camera();
            }
            if (ImGui::MenuItem("Create Point Light")) {
                m_vm->request_create_light(light_type::point);
            }
            if (ImGui::MenuItem("Create Spot Light")) {
                m_vm->request_create_light(light_type::spot);
            }
            if (ImGui::MenuItem("Create Directional Light")) {
                m_vm->request_create_light(light_type::directional);
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("UI"))
            {
                if (ImGui::MenuItem("Canvas"))
                {
                    registry& reg = m_vm->get_registry();
                    entity canvas = ensure_canvas_exists(reg);
                    m_vm->request_select(canvas, false);
                }
                if (ImGui::MenuItem("Text"))
                {
                    registry& reg = m_vm->get_registry();
                    entity e = reg.create_entity();
                    reg.add_component<name_component>(e, name_component{ "Text" });
                    reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(160.0f, 40.0f) });
                    reg.add_component<ui_text_component>(e, ui_text_component{ "New Text" });
                    prepare_ui_entity(e, reg, false);
                    m_vm->request_select(e, false);
                }
                if (ImGui::MenuItem("Image"))
                {
                    registry& reg = m_vm->get_registry();
                    entity e = reg.create_entity();
                    reg.add_component<name_component>(e, name_component{ "Image" });
                    reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(100.0f, 100.0f) });
                    reg.add_component<ui_image_component>(e, ui_image_component{});
                    prepare_ui_entity(e, reg, false);
                    m_vm->request_select(e, false);
                }
                if (ImGui::MenuItem("Button"))
                {
                    registry& reg = m_vm->get_registry();
                    entity e = reg.create_entity();
                    reg.add_component<name_component>(e, name_component{ "Button" });
                    reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(160.0f, 48.0f) });
                    reg.add_component<ui_image_component>(e, ui_image_component{});
                    reg.add_component<ui_button_component>(e, ui_button_component{});
                    prepare_ui_entity(e, reg, false);

                    entity textChild = reg.create_entity();
                    reg.add_component<name_component>(textChild, name_component{ "Text" });
                    reg.add_component<rect_transform_component>(textChild, rect_transform_component{
                        glm::vec2(0.0f), glm::vec2(160.0f, 48.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.5f, 0.5f)
                    });
                    reg.add_component<ui_text_component>(textChild, ui_text_component{
                        "Button", "EngineResources/Fonts/Roboto-Regular.ttf", 20.0f, glm::vec4(0.12f, 0.12f, 0.12f, 1.0f)
                    });
                    attach_child_to_parent(reg, textChild, e);

                    m_vm->request_select(e, false);
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Paste", nullptr, false, m_vm->can_paste())) {
                m_vm->request_paste();
            }
            ImGui::EndPopup();
        }

        // ==========================================
        // AĞAÇ ÇİZİMİ (DRAW HIERARCHY)
        // ==========================================
        const auto root_nodes = m_vm->get_root_nodes();
        EditorCamera* activeCam = get_camera();
        for (const auto& node : root_nodes) {
            draw_entity_node(m_vm.get(), node, m_gizmo_settings, activeCam);
        }

        // ==========================================
        // KÖKE BIRAKMA ALANI (DROP TO ROOT)
        // ==========================================
        ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
        if (dropZoneSize.y < 40.0f) {
            dropZoneSize.y = 40.0f;
        }
        ImGui::InvisibleButton("##HierarchyRootDropZone", dropZoneSize);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VESTA_ENTITY")) {
                const size_t count = payload->DataSize / sizeof(entity);
                const entity* draggedEntities = static_cast<const entity*>(payload->Data);
                std::vector<entity> toReparent;
                toReparent.reserve(count);
                for (size_t i = 0; i < count; ++i) {
                    if (draggedEntities[i] != 0) {
                        toReparent.push_back(draggedEntities[i]);
                    }
                }
                m_vm->request_set_parent_batch(toReparent, 0); // 0 (Kök/Root) ebeveyne ata
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
    }

    const std::string& hierarchy_panel::get_title() const {
        return m_title;
    }

    bool hierarchy_panel::is_open() const {
        return m_is_open;
    }

    void hierarchy_panel::set_open(bool open) {
        m_is_open = open;
    }

} // namespace vesta
