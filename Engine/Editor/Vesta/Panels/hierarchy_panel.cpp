#include "hierarchy_panel.h"
#include <imgui.h>

namespace vesta {

    void draw_entity_node(hierarchy_view_model* vm, const hierarchy_node_data& node)
    {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (!node.has_children)
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (node.is_selected)
            flags |= ImGuiTreeNodeFlags_Selected;

        bool opened = false;

        if (vm->is_renaming(node.id))
        {
            static char rename_buffer[256];
            static entity editing_entity = 0;

            if (editing_entity != node.id)
            {
                editing_entity = node.id;

                strncpy_s(
                    rename_buffer,
                    vm->get_rename_buffer().c_str(),
                    sizeof(rename_buffer));

                ImGui::SetKeyboardFocusHere();
            }

            ImGui::SetNextItemWidth(-1.0f);

            bool commit = ImGui::InputText(
                "##Rename",
                rename_buffer,
                sizeof(rename_buffer),
                ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll);

            if (commit)
            {
                vm->get_rename_buffer() = rename_buffer;
                vm->commit_rename();
                editing_entity = 0;
            }

            if (ImGui::IsItemActive() &&
                ImGui::IsKeyPressed(ImGuiKey_Escape))
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

            return;
        }
        else
        {
            opened = ImGui::TreeNodeEx(
                reinterpret_cast<void*>(static_cast<uintptr_t>(node.id)),
                flags,
                "%s",
                node.name.c_str());
        }

        //---------------------------------------------------
        // Selection
        //---------------------------------------------------

        if (ImGui::IsItemClicked())
        {
            vm->request_select(node.id, ImGui::GetIO().KeyCtrl);
        }

        //---------------------------------------------------
        // Drag Source
        //---------------------------------------------------

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(
                "VESTA_ENTITY",
                &node.id,
                sizeof(entity));

            ImGui::Text("%s", node.name.c_str());

            ImGui::EndDragDropSource();
        }

        //---------------------------------------------------
        // Drag Target
        //---------------------------------------------------

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("VESTA_ENTITY"))
            {
                entity dragged =
                    *static_cast<const entity*>(payload->Data);

                if (dragged != node.id)
                {
                    vm->request_set_parent(dragged, node.id);
                }
            }

            ImGui::EndDragDropTarget();
        }

        //---------------------------------------------------
        // Double Click Rename
        //---------------------------------------------------

        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            vm->start_rename(node.id);
        }

        //---------------------------------------------------
        // Context Menu
        //---------------------------------------------------

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Rename"))
            {
                vm->start_rename(node.id);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Copy"))
            {
                vm->request_copy(node.id);
            }

            if (ImGui::MenuItem("Duplicate"))
            {
                vm->request_duplicate(node.id);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete"))
            {
                vm->request_destroy_entity(node.id);
            }

            ImGui::EndPopup();
        }

        //---------------------------------------------------
        // Children
        //---------------------------------------------------

        if (opened)
        {
            for (entity child : node.children)
            {
                draw_entity_node(
                    vm,
                    vm->get_node_data(child));
            }

            ImGui::TreePop();
        }

    }

    void hierarchy_panel::render()
    {
        if (!m_is_open)
            return;

        ImGui::Begin(m_title.c_str(), &m_is_open);

        //-------------------------------------------------------
        // Shortcuts
        //-------------------------------------------------------

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            bool ctrl =
                ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ||
                ImGui::IsKeyDown(ImGuiKey_RightCtrl);

            if (ctrl)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_C))
                    m_vm->request_copy_selected();

                if (ImGui::IsKeyPressed(ImGuiKey_D))
                    m_vm->request_duplicate_selected();

                if (ImGui::IsKeyPressed(ImGuiKey_V))
                    m_vm->request_paste();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Delete))
            {
                m_vm->request_delete_selected();
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F2))
            {
                entity selected = m_vm->get_selected_entity();

                if (selected != 0)
                    m_vm->start_rename(selected);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                if (m_vm->get_renaming_entity() != 0)
                    m_vm->cancel_rename();
            }
        }

        //-------------------------------------------------------
        // Toolbar
        //-------------------------------------------------------

        if (ImGui::Button("Create Object"))
        {
            m_vm->request_create_entity();
        }

        ImGui::Separator();

        //-------------------------------------------------------
        // Empty Area Context Menu
        //-------------------------------------------------------

        if (ImGui::BeginPopupContextWindow(
            "Hierarchy_Context",
            ImGuiPopupFlags_MouseButtonRight |
            ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem(
                "Paste",
                nullptr,
                false,
                m_vm->can_paste()))
            {
                m_vm->request_paste();
            }

            ImGui::EndPopup();
        }

        //-------------------------------------------------------
        // Draw Hierarchy
        //-------------------------------------------------------

        const auto root_nodes = m_vm->get_root_nodes();

        for (const auto& node : root_nodes)
        {
            draw_entity_node(
                m_vm.get(),
                node);
        }

        //-------------------------------------------------------
        // Drop To Root
        //-------------------------------------------------------

        ImGui::InvisibleButton(
            "##HierarchyRootDropZone",
            ImGui::GetContentRegionAvail());

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("VESTA_ENTITY"))
            {
                entity dragged =
                    *static_cast<const entity*>(payload->Data);

                m_vm->request_set_parent(dragged, 0);
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::End();
    }

}