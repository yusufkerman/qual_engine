/**
 * @file asset_browser_panel.cpp
 * @brief Varlık Tarayıcısı (Asset Browser) panelinin ImGui ile çizim işlemlerini barındırır.
 */

#include "asset_browser_panel.h"
#include "../ViewModels/asset_browser_view_model.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <windows.h>
#include <shellapi.h>
#include "../../Project/engine_settings.h"
#include "../../Project/project.h"

namespace {
    const char* get_node_icon(vesta::asset_type type) {
        switch (type) {
        case vesta::asset_type::folder:   return "[DIR] ";
        case vesta::asset_type::material: return "[MAT] ";
        case vesta::asset_type::texture:  return "[TEX] ";
        case vesta::asset_type::model:    return "[MESH] ";
        default:                          return "[FILE] ";
        }
    }
}

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

        // Yenileme (Refresh) Butonu
        if (ImGui::Button("Refresh")) {
            m_vm.refresh();
        }

        ImGui::Separator();

        // Boş alana sol tıklanınca mevcut seçimi temizle
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            if (!ImGui::IsAnyItemHovered()) {
                m_vm.clear_selection();
            }
        }

        // ========================================================================
        // BOŞ ALANA SAĞ TIK MENÜSÜ (KÖK DİZİNE VARLIK EKLEME)
        // ========================================================================
        if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("[+] Create Material...")) {
                m_context_target_path = m_vm.root().full_path;
                m_open_create_popup = true;
                snprintf(m_create_buffer, sizeof(m_create_buffer), "NewMaterial");
            }
            ImGui::EndPopup();
        }

        // ========================================================================
        // KÖK (ROOT) DÜĞÜMÜN ÇİZİMİ
        // ========================================================================
        const auto& root = m_vm.root();
        ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

        const std::string rootPathStr = root.full_path.string();
        if (m_vm.is_selected(rootPathStr) || m_vm.is_selected("Assets")) {
            root_flags |= ImGuiTreeNodeFlags_Selected;
        }

        const std::string rootLabel = std::string("[DIR] ") + (root.name.empty() ? "Assets" : root.name);
        bool root_open = ImGui::TreeNodeEx(rootLabel.c_str(), root_flags);
        if (ImGui::IsItemClicked()) {

        }

        if (ImGui::BeginPopupContextItem("RootContextMenu")) {

            m_context_target_path = root.full_path;

            if (ImGui::MenuItem("[+] Create Material Here...")) {
                m_open_create_popup = true;
                snprintf(m_create_buffer, sizeof(m_create_buffer), "NewMaterial");
            }
            if (ImGui::MenuItem("[+] Create C++ Script Here...")) {
                m_open_create_script_popup = true;
                snprintf(m_create_script_buffer, sizeof(m_create_script_buffer), "PlayerController");
            }
            if (ImGui::MenuItem("[+] Create Folder Here...")) {
                m_open_create_folder_popup = true;
                snprintf(m_create_folder_buffer, sizeof(m_create_folder_buffer), "NewFolder");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Open in Code Editor")) {
                std::string project_root = project::project_manager::instance().get_project_root();
                project::engine_settings::instance().open_in_editor(root.full_path.string(), project_root);
            }
            if (ImGui::MenuItem("Show in Explorer")) {
                ShellExecuteA(NULL, "open", "explorer.exe", root.full_path.string().c_str(), NULL, SW_SHOW);
            }
            ImGui::EndPopup();
        }

        if (root_open) {
            for (const auto& child : root.children) {
                draw_node(child);
            }
            ImGui::TreePop();
        }

        // ========================================================================
        // MODAL POPUP: YENİ MATERYAL OLUŞTURMA
        // ========================================================================
        if (m_open_create_popup) {
            ImGui::OpenPopup("Create Asset Name");
            m_open_create_popup = false;
        }

        if (ImGui::BeginPopupModal("Create Asset Name", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter material name:");
            bool enter_pressed = ImGui::InputText("##create_name", m_create_buffer, sizeof(m_create_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Spacing();

            if (ImGui::Button("Create", ImVec2(120, 0)) || enter_pressed) {
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
        // MODAL POPUP: YENİDEN ADLANDIRMA (RENAME)
        // ========================================================================
        if (m_open_rename_popup) {
            ImGui::OpenPopup("Rename Asset");
            m_open_rename_popup = false;
        }

        if (ImGui::BeginPopupModal("Rename Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Rename '%s' to:", m_context_target_path.filename().string().c_str());
            bool rename_enter = ImGui::InputText("##rename_input", m_rename_buffer, sizeof(m_rename_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0)) || rename_enter) {
                m_vm.request_rename(m_context_target_path, m_rename_buffer);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // MODAL POPUP: YENİ C++ SCRİPT OLUŞTURMA
        // ========================================================================
        if (m_open_create_script_popup) {
            ImGui::OpenPopup("Create C++ Script");
            m_open_create_script_popup = false;
        }

        if (ImGui::BeginPopupModal("Create C++ Script", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter script / class name:");
            bool script_enter = ImGui::InputText("##create_script_name", m_create_script_buffer, sizeof(m_create_script_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::TextDisabled("Will generate %s.h and %s.cpp inheriting gameplay::script", m_create_script_buffer, m_create_script_buffer);
            ImGui::Spacing();

            if (ImGui::Button("Create & Open", ImVec2(120, 0)) || script_enter) {
                m_vm.request_create_script(m_context_target_path, m_create_script_buffer);
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // MODAL POPUP: YENİ KLASÖR OLUŞTURMA
        // ========================================================================
        if (m_open_create_folder_popup) {
            ImGui::OpenPopup("Create Folder");
            m_open_create_folder_popup = false;
        }

        if (ImGui::BeginPopupModal("Create Folder", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter folder name:");
            bool folder_enter = ImGui::InputText("##create_folder_name", m_create_folder_buffer, sizeof(m_create_folder_buffer), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::Spacing();

            if (ImGui::Button("Create", ImVec2(120, 0)) || folder_enter) {
                m_vm.request_create_folder(m_context_target_path, m_create_folder_buffer);
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

        // Aynı isimde klasörler olabileceği için ImGui state karışmasını engelliyoruz
        ImGui::PushID(path_str.c_str());

        std::string prefix = get_node_icon(entry.type);
        std::string label = prefix + entry.name;
        bool is_selected = m_vm.is_selected(path_str);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (entry.type != asset_type::folder) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if (is_selected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool is_open = ImGui::TreeNodeEx(label.c_str(), flags);

        // ========================================================================
        // FARE ETKİLEŞİMLERİ (ÇİFT TIKLAMA VE SAĞ TIK SEÇİMİ)
        // ========================================================================
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_vm.select_and_notify(path_str);

                std::string ext = entry.full_path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cs")
                {
                    std::string project_root;
                    if (project::project_manager::instance().has_active_project()) {
                        project_root = project::project_manager::instance().get_project_root();
                    }
                    project::engine_settings::instance().open_in_editor(entry.full_path.string(), project_root);
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                m_vm.select(path_str);
            }
        }

        // ========================================================================
        // ÖĞEYE ÖZEL İÇERİK MENÜSÜ (CONTEXT MENU)
        // ========================================================================
        if (ImGui::BeginPopupContextItem("ItemContextMenu")) {
            m_vm.select_and_notify(path_str);

            if (entry.type == asset_type::folder) {
                if (ImGui::MenuItem("[+] Create Material Here...")) {
                    m_context_target_path = entry.full_path;
                    m_open_create_popup = true;
                    snprintf(m_create_buffer, sizeof(m_create_buffer), "NewMaterial");
                }
                if (ImGui::MenuItem("[+] Create C++ Script Here...")) {
                    m_context_target_path = entry.full_path;
                    m_open_create_script_popup = true;
                    snprintf(m_create_script_buffer, sizeof(m_create_script_buffer), "PlayerController");
                }
                if (ImGui::MenuItem("[+] Create Folder Here...")) {
                    m_context_target_path = entry.full_path;
                    m_open_create_folder_popup = true;
                    snprintf(m_create_folder_buffer, sizeof(m_create_folder_buffer), "NewFolder");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Open in Code Editor")) {
                    std::string project_root;
                    if (project::project_manager::instance().has_active_project()) {
                        project_root = project::project_manager::instance().get_project_root();
                    }
                    project::engine_settings::instance().open_in_editor(entry.full_path.string(), project_root);
                }
                if (ImGui::MenuItem("Show in Explorer")) {
                    ShellExecuteA(NULL, "open", "explorer.exe", entry.full_path.string().c_str(), NULL, SW_SHOW);
                }
                ImGui::Separator();
            }
            else {
                if (ImGui::MenuItem("[+] Create Material in Folder...")) {
                    m_context_target_path = entry.full_path.parent_path();
                    m_open_create_popup = true;
                    snprintf(m_create_buffer, sizeof(m_create_buffer), "NewMaterial");
                }
                if (ImGui::MenuItem("[+] Create C++ Script in Folder...")) {
                    m_context_target_path = entry.full_path.parent_path();
                    m_open_create_script_popup = true;
                    snprintf(m_create_script_buffer, sizeof(m_create_script_buffer), "PlayerController");
                }
                if (ImGui::MenuItem("[+] Create Folder in Folder...")) {
                    m_context_target_path = entry.full_path.parent_path();
                    m_open_create_folder_popup = true;
                    snprintf(m_create_folder_buffer, sizeof(m_create_folder_buffer), "NewFolder");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Open in Code Editor")) {
                    std::string project_root;
                    if (project::project_manager::instance().has_active_project()) {
                        project_root = project::project_manager::instance().get_project_root();
                    }
                    project::engine_settings::instance().open_in_editor(entry.full_path.string(), project_root);
                }
                if (ImGui::MenuItem("Show in Explorer")) {
                    ShellExecuteA(NULL, "open", "explorer.exe", entry.full_path.string().c_str(), NULL, SW_SHOW);
                }
                ImGui::Separator();
            }

            if (ImGui::MenuItem("Rename")) {
                m_context_target_path = entry.full_path;
                snprintf(m_rename_buffer, sizeof(m_rename_buffer), "%s", entry.full_path.stem().string().c_str());
                m_open_rename_popup = true;
            }

            if (ImGui::MenuItem("Duplicate / Copy")) {
                m_vm.request_duplicate(entry.full_path);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                m_vm.request_delete(entry.full_path);
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // SÜRÜKLE VE BIRAK (DRAG & DROP) KAYNAĞI
        // ========================================================================
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            vesta::asset_drag_payload payload{};
            std::string ext = entry.full_path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".json" || ext == ".amat") {
                payload.type = vesta::asset_payload_type::material;
            }
            else if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".dds") {
                payload.type = vesta::asset_payload_type::texture;
            }
            else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".dae") {
                payload.type = vesta::asset_payload_type::model;
            }
            else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp") {
                payload.type = vesta::asset_payload_type::shader;
            }
            else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
                payload.type = vesta::asset_payload_type::audio;
            }
            else {
                payload.type = vesta::asset_payload_type::unspecified; // Fallback
            }

            // Dosya yolunu bellekte kopyalanabilir (POD) bir şekilde pakete ekle
            snprintf(payload.path, sizeof(payload.path), "%s", path_str.c_str());
            payload.guid = entry.guid;

            ImGui::SetDragDropPayload("ARIA_ASSET_PAYLOAD", &payload, sizeof(payload));

            // Sürüklerken farenin altında görünecek olan önizleme metni
            ImGui::Text("%s %s", (entry.type == asset_type::folder ? "[DIR]" : "[FILE]"), entry.name.c_str());
            ImGui::EndDragDropSource();
        }

        // ========================================================================
        // SÜRÜKLE VE BIRAK (DRAG & DROP) HEDEFİ
        // ========================================================================
        if (entry.type == asset_type::folder) {
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* imgui_payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD"))
                {
                    auto* data = (const vesta::asset_drag_payload*)imgui_payload->Data;
                    std::filesystem::path src(data->path);

                    // ViewModel üzerinden dosya taşıma (move) isteği yolla
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
