#include "asset_browser_panel.h"
#include "../ViewModels/asset_browser_view_model.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace vesta {

    asset_browser_panel::asset_browser_panel(asset_browser_view_model& vm)
        : m_vm(vm)
    {
    }

    void asset_browser_panel::init() {
        m_vm.refresh();
    }

    void asset_browser_panel::render() {
        ImGui::Begin("Asset Browser");

        // 🔄 REFRESH BAR
        if (ImGui::Button("Refresh")) {
            m_vm.refresh();
        }

        ImGui::Separator();

        // Boş alana sol tıklanınca seçimi temizle
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            if (!ImGui::IsAnyItemHovered()) {
                m_vm.clear_selection();
            }
        }

        // ========================================================================
        // 🔥 BOŞ ALANA SAĞ TIKLANDIĞINDA (KÖK DİZİNE YENİ ASSET OLUŞTURMA)
        // ========================================================================
        if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("➕ Create Material...")) {
                m_context_target_path = "Assets";
                m_open_create_popup = true;
                snprintf(m_create_buffer, sizeof(m_create_buffer), "NewMaterial");
            }
            ImGui::EndPopup();
        }

        // 🔥 ROOT DÜĞÜMÜNÜ ÇİZ
        const auto& root = m_vm.root();
        ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (m_vm.is_selected("Assets")) root_flags |= ImGuiTreeNodeFlags_Selected;

        bool root_open = ImGui::TreeNodeEx("📁 Assets", root_flags);
        if (ImGui::IsItemClicked()) {
            m_vm.select("Assets");
        }

        if (root_open) {
            for (const auto& child : root.children) {
                draw_node(child);
            }
            ImGui::TreePop();
        }

        // ========================================================================
        // 🛠️ MODAL POPUP: YENİ ASSET OLUŞTURMA (İSİM SORARAK)
        // ========================================================================
        if (m_open_create_popup) {
            ImGui::OpenPopup("Create Asset Name");
            m_open_create_popup = false;
        }

        if (ImGui::BeginPopupModal("Create Asset Name", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter material name:");
            ImGui::InputText("##create_name", m_create_buffer, sizeof(m_create_buffer));
            ImGui::Spacing();

            if (ImGui::Button("Create", ImVec2(120, 0))) {
                m_vm.request_create_material(m_context_target_path, m_create_buffer);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // 🛠️ MODAL POPUP: RENAME (YENİDEN ADLANDIRMA)
        // ========================================================================
        if (m_open_rename_popup) {
            ImGui::OpenPopup("Rename Asset");
            m_open_rename_popup = false;
        }

        if (ImGui::BeginPopupModal("Rename Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Rename '%s' to:", m_context_target_path.filename().string().c_str());
            ImGui::InputText("##rename_input", m_rename_buffer, sizeof(m_rename_buffer));
            ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                m_vm.request_rename(m_context_target_path, m_rename_buffer);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void asset_browser_panel::draw_node(const asset_file_entry& entry) {
        std::string path_str = entry.full_path.string();
        ImGui::PushID(path_str.c_str());

        std::string prefix = (entry.type == asset_type::folder) ? "📁 " : "📄 ";
        std::string label = prefix + entry.name;
        bool is_selected = m_vm.is_selected(path_str);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (entry.type != asset_type::folder) flags |= ImGuiTreeNodeFlags_Leaf;
        if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

        bool is_open = ImGui::TreeNodeEx(label.c_str(), flags);

        // ========================================================================
        // 🚀 GÜVENLİ ÇİFT TIKLAMA SEÇİM MANTIĞI
        // ========================================================================
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_vm.select_and_notify(path_str);
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                m_vm.select(path_str);
            }
        }

        // ========================================================================
        // 🔥 ÖĞEYE ÖZEL SAĞ TIK MENÜSÜ (CONTEXT MENU)
        // ========================================================================
        if (ImGui::BeginPopupContextItem("ItemContextMenu")) {
            m_vm.select_and_notify(path_str);

            if (entry.type == asset_type::folder) {
                if (ImGui::MenuItem("➕ Create Material Here...")) {
                    m_context_target_path = entry.full_path;
                    m_open_create_popup = true;
                    snprintf(m_create_buffer, sizeof(m_create_buffer), "NewMaterial");
                }
                ImGui::Separator();
            }

            if (ImGui::MenuItem("📝 Rename")) {
                m_context_target_path = entry.full_path;
                snprintf(m_rename_buffer, sizeof(m_rename_buffer), "%s", entry.full_path.stem().string().c_str());
                m_open_rename_popup = true;
            }

            if (ImGui::MenuItem("👯 Duplicate / Copy")) {
                m_vm.request_duplicate(entry.full_path);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("🗑️ Delete")) {
                m_vm.request_delete(entry.full_path);
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // 🚀 SÜRÜKLEME KAYNAĞI (DRAG SOURCE)
        // ========================================================================
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            vesta::asset_drag_payload payload{};
            std::string ext = entry.full_path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".json") {
                payload.type = vesta::asset_payload_type::material;
            }
            else if (ext == ".png" || ext == ".jpg" || ext == ".tga") {
                payload.type = vesta::asset_payload_type::texture;
            }
            else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf") {
                payload.type = vesta::asset_payload_type::model;
            }
            else {
                payload.type = vesta::asset_payload_type::model;
            }

            snprintf(payload.path, sizeof(payload.path), "%s", path_str.c_str());
            payload.guid = entry.guid;

            ImGui::SetDragDropPayload("ARIA_ASSET_PAYLOAD", &payload, sizeof(payload));

            ImGui::Text("%s %s", (entry.type == asset_type::folder ? "📁" : "📄"), entry.name.c_str());
            ImGui::EndDragDropSource();
        }

        // ========================================================================
        // BIRAKMA HEDEFİ (DROP TARGET)
        // ========================================================================
        if (entry.type == asset_type::folder) {
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* imgui_payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD"))
                {
                    auto* data = (const vesta::asset_drag_payload*)imgui_payload->Data;
                    std::filesystem::path src(data->path);
                    m_vm.request_move(src, entry.full_path);
                }
                ImGui::EndDragDropTarget();
            }
        }

        if (is_open) {
            if (entry.type == asset_type::folder) {
                for (const auto& child : entry.children) {
                    draw_node(child);
                }
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    const std::string& asset_browser_panel::get_title() const {
        return m_title;
    }

    bool asset_browser_panel::is_open() const {
        return m_is_open;
    }

    void asset_browser_panel::set_open(bool open) {
        m_is_open = open;
    }

} // namespace vesta