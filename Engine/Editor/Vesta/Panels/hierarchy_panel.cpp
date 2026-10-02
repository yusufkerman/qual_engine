/**
 * @file hierarchy_panel.cpp
 * @brief Sahne hiyerarşisi panelinin ImGui çizim ve kısayol dinleme işlemlerini barındırır.
 */

#include "hierarchy_panel.h"
#include <imgui.h>
#include <cstdio> // snprintf için

namespace vesta {

    hierarchy_panel::hierarchy_panel(IEditableWorld& world, registry& reg, editor_selection_manager& selection)
        : m_vm(std::make_unique<hierarchy_view_model>(world, reg, selection))
    {
    }

    /**
     * @brief Ağaç yapısındaki tekil bir varlığı (Entity) çizer.
     * @note Static tanımlanarak sadece bu derleme birimine (translation unit) özel hale getirilmiştir.
     */
    static void draw_entity_node(hierarchy_view_model* vm, const hierarchy_node_data& node)
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

                // Güvenli ve cross-platform (MSVC/GCC/Clang) metin kopyalama
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
            opened = ImGui::TreeNodeEx(
                reinterpret_cast<void*>(static_cast<uintptr_t>(node.id)),
                flags,
                "%s",
                node.name.c_str());
        }

        // ==========================================
        // SEÇİM (SELECTION)
        // ==========================================
        if (ImGui::IsItemClicked()) {
            vm->request_select(node.id, ImGui::GetIO().KeyCtrl);
        }

        // ==========================================
        // SÜRÜKLEME KAYNAĞI (DRAG SOURCE)
        // ==========================================
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("VESTA_ENTITY", &node.id, sizeof(entity));
            ImGui::Text("%s", node.name.c_str());
            ImGui::EndDragDropSource();
        }

        // ==========================================
        // BIRAKMA HEDEFİ (DROP TARGET)
        // ==========================================
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VESTA_ENTITY")) {
                entity dragged = *static_cast<const entity*>(payload->Data);
                if (dragged != node.id) {
                    vm->request_set_parent(dragged, node.id);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // ==========================================
        // ÇİFT TIKLA YENİDEN ADLANDIR
        // ==========================================
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            vm->start_rename(node.id);
        }

        // ==========================================
        // İÇERİK MENÜSÜ (CONTEXT MENU)
        // ==========================================
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename")) {
                vm->start_rename(node.id);
            }
            ImGui::Separator();
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
                draw_entity_node(vm, vm->get_node_data(child));
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

            if (ctrl) {
                if (ImGui::IsKeyPressed(ImGuiKey_C)) m_vm->request_copy_selected();
                if (ImGui::IsKeyPressed(ImGuiKey_D)) m_vm->request_duplicate_selected();
                if (ImGui::IsKeyPressed(ImGuiKey_V)) m_vm->request_paste();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                m_vm->request_delete_selected();
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

        ImGui::Separator();

        // ==========================================
        // BOŞ ALAN İÇERİK MENÜSÜ (EMPTY AREA CONTEXT)
        // ==========================================
        if (ImGui::BeginPopupContextWindow("Hierarchy_Context", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Paste", nullptr, false, m_vm->can_paste())) {
                m_vm->request_paste();
            }
            ImGui::EndPopup();
        }

        // ==========================================
        // AĞAÇ ÇİZİMİ (DRAW HIERARCHY)
        // ==========================================
        const auto root_nodes = m_vm->get_root_nodes();
        for (const auto& node : root_nodes) {
            draw_entity_node(m_vm.get(), node);
        }

        // ==========================================
        // KÖKE BIRAKMA ALANI (DROP TO ROOT)
        // ==========================================
        ImGui::InvisibleButton("##HierarchyRootDropZone", ImGui::GetContentRegionAvail());

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VESTA_ENTITY")) {
                entity dragged = *static_cast<const entity*>(payload->Data);
                m_vm->request_set_parent(dragged, 0); // 0 (Kök/Root) ebeveyne ata
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