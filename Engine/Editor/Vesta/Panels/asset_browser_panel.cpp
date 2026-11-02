/**
 * @file asset_browser_panel.cpp
 * @brief Varlık Tarayıcısı (Asset Browser) panelinin ImGui ile çizim işlemlerini barındırır.
 */

#include "asset_browser_panel.h"
#include "../../../Core/path_utils.h"
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

    std::string to_lower_str(std::string_view s) {
        std::string res;
        res.reserve(s.size());
        for (char c : s) {
            res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return res;
    }

    bool entry_matches_search(const vesta::asset_file_entry& entry, const std::string& queryLower) {
        if (queryLower.empty()) return true;
        std::string nameLower = to_lower_str(entry.name);
        if (nameLower.find(queryLower) != std::string::npos) return true;
        if (entry.type == vesta::asset_type::folder) {
            for (const auto& child : entry.children) {
                if (entry_matches_search(child, queryLower)) return true;
            }
        }
        return false;
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
        if (!m_is_open)
            return;

        ImGui::Begin(m_title.c_str(), &m_is_open);

        // ========================================================================
        // KISAYOLLAR (SHORTCUTS)
        // ========================================================================
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            const std::string& sel = m_vm.selected_path();
            if (!sel.empty()) {
                if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                    m_context_target_path = qual::path_from_utf8(sel);
                    const std::string stem_u8 = std::filesystem::is_directory(m_context_target_path)
                        ? qual::path_to_utf8(m_context_target_path.filename())
                        : qual::path_to_utf8(m_context_target_path.stem());
                    snprintf(m_rename_buffer, sizeof(m_rename_buffer), "%s", stem_u8.c_str());
                    m_open_rename_popup = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                    m_context_target_path = qual::path_from_utf8(sel);
                    m_vm.request_delete(m_context_target_path);
                }
                if ((ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) && ImGui::IsKeyPressed(ImGuiKey_D)) {
                    m_context_target_path = qual::path_from_utf8(sel);
                    m_vm.request_duplicate(m_context_target_path);
                }
            }
        }

        // ========================================================================
        // ÜST ARAÇ ÇUBUĞU (TOOLBAR: REFRESH + ARAMA KUTUSU)
        // ========================================================================
        if (ImGui::Button("Refresh")) {
            m_vm.refresh();
        }
        ImGui::SameLine();
        const float clearBtnWidth = (m_search_buffer[0] != '\0') ? 28.0f : 0.0f;
        const float searchInputWidth = std::max(60.0f, ImGui::GetContentRegionAvail().x - clearBtnWidth);
        ImGui::SetNextItemWidth(searchInputWidth);
        ImGui::InputTextWithHint("##AssetSearch", "Search assets...", m_search_buffer, sizeof(m_search_buffer));
        if (m_search_buffer[0] != '\0') {
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                m_search_buffer[0] = '\0';
            }
        }

        ImGui::Separator();

        // Boş alana sol tıklanınca mevcut seçimi temizle
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            if (!ImGui::IsAnyItemHovered()) {
                m_vm.clear_selection();
            }
        }

        // ========================================================================
        // KÖK (ROOT) DÜĞÜMÜN ÇİZİMİ
        // ========================================================================
        const auto& root = m_vm.root();
        ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

        const std::string rootPathStr = qual::path_to_utf8(root.full_path);
        if (m_vm.is_selected(rootPathStr) || m_vm.is_selected("Assets")) {
            root_flags |= ImGuiTreeNodeFlags_Selected;
        }

        std::string projName = project::project_manager::instance().has_active_project()
            ? project::project_manager::instance().get_project_name()
            : "";
        std::string rootLabel = projName.empty()
            ? std::string("[DIR] ") + (root.name.empty() ? "Assets" : root.name)
            : std::string("[DIR] ") + projName + " (Assets)";

        ImGui::PushID(rootPathStr.c_str());
        bool root_open = ImGui::TreeNodeEx(rootLabel.c_str(), root_flags);

        // Kök klasöre bırakma hedefi (Drag & Drop Target to Root)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* imgui_payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD")) {
                auto* data = (const vesta::asset_drag_payload*)imgui_payload->Data;
                std::filesystem::path src = qual::path_from_utf8(data->path);
                m_vm.request_move(src, root.full_path);
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            m_vm.select(rootPathStr);
            m_context_target_path = root.full_path;
            ImGui::OpenPopup("AssetRootContextMenu");
        }

        if (ImGui::BeginPopup("AssetRootContextMenu") || ImGui::BeginPopupContextItem("##AssetRootContextItem")) {
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
                std::string project_root = project::project_manager::instance().has_active_project()
                    ? project::project_manager::instance().get_project_root()
                    : "";
                project::engine_settings::instance().open_in_editor(qual::path_to_utf8(root.full_path), project_root);
            }
            if (ImGui::MenuItem("Show in Explorer")) {
                ShellExecuteW(NULL, L"open", L"explorer.exe", root.full_path.c_str(), NULL, SW_SHOW);
            }
            ImGui::EndPopup();
        }

        if (root_open) {
            std::string qLower = to_lower_str(m_search_buffer);
            for (const auto& child : root.children) {
                if (qLower.empty() || entry_matches_search(child, qLower)) {
                    draw_node(child);
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();

        // ========================================================================
        // BOŞ ALANA SAĞ TIK MENÜSÜ (KÖK DİZİNE VEYA GEÇERLİ DİZİNE VARLIK EKLEME)
        // ========================================================================
        if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            m_context_target_path = m_vm.root().full_path;

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
                std::string project_root = project::project_manager::instance().has_active_project()
                    ? project::project_manager::instance().get_project_root()
                    : "";
                project::engine_settings::instance().open_in_editor(qual::path_to_utf8(m_context_target_path), project_root);
            }
            if (ImGui::MenuItem("Show in Explorer")) {
                ShellExecuteW(NULL, L"open", L"explorer.exe", m_context_target_path.c_str(), NULL, SW_SHOW);
            }
            ImGui::EndPopup();
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
            const std::string target_filename_u8 = qual::path_to_utf8(m_context_target_path.filename());
            ImGui::Text("Rename '%s' to:", target_filename_u8.c_str());
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
        std::string path_str = qual::path_to_utf8(entry.full_path);

        ImGui::PushID(path_str.c_str());

        std::string prefix = get_node_icon(entry.type);
        std::string label = prefix + entry.name;
        bool is_selected = m_vm.is_selected(path_str);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (entry.type != asset_type::folder) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (is_selected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (m_search_buffer[0] != '\0') {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        bool is_open = ImGui::TreeNodeEx(label.c_str(), flags);

        // ========================================================================
        // FARE ETKİLEŞİMLERİ (TEK TIK SEÇİM VE ÇİFT TIKLAMA INSPECTOR BİLDİRİMİ)
        // ========================================================================
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
            m_vm.select(path_str);
        }

        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_vm.select_and_notify(path_str);

                std::string ext = qual::path_to_utf8(entry.full_path.extension());
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".cs") {
                    std::string project_root;
                    if (project::project_manager::instance().has_active_project()) {
                        project_root = project::project_manager::instance().get_project_root();
                    }
                    project::engine_settings::instance().open_in_editor(path_str, project_root);
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                m_vm.select(path_str);
            }
        }

        // ========================================================================
        // SÜRÜKLE VE BIRAK (DRAG & DROP) KAYNAĞI
        // ========================================================================
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            vesta::asset_drag_payload payload{};
            std::string ext = qual::path_to_utf8(entry.full_path.extension());
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".json" || ext == ".amat") {
                payload.type = vesta::asset_payload_type::material;
            }
            else if (ext == ".png" || ext == ".jpg" || ext == ".tga" || ext == ".dds") {
                payload.type = vesta::asset_payload_type::texture;
            }
            else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
                payload.type = vesta::asset_payload_type::model;
            }
            else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp") {
                payload.type = vesta::asset_payload_type::shader;
            }
            else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") {
                payload.type = vesta::asset_payload_type::audio;
            }
            else {
                payload.type = vesta::asset_payload_type::unspecified;
            }

            snprintf(payload.path, sizeof(payload.path), "%s", path_str.c_str());
            payload.guid = entry.guid;

            ImGui::SetDragDropPayload("ARIA_ASSET_PAYLOAD", &payload, sizeof(payload));
            ImGui::Text("%s %s", (entry.type == asset_type::folder ? "[DIR]" : "[FILE]"), entry.name.c_str());
            ImGui::EndDragDropSource();
        }

        // ========================================================================
        // SÜRÜKLE VE BIRAK (DRAG & DROP) HEDEFİ: KLASÖRE VEYA DOSYA YANINA TAŞIMA
        // ========================================================================
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* imgui_payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD"))
            {
                auto* data = (const vesta::asset_drag_payload*)imgui_payload->Data;
                std::filesystem::path src = qual::path_from_utf8(data->path);
                std::filesystem::path dst_dir = (entry.type == asset_type::folder) ? entry.full_path : entry.full_path.parent_path();
                m_vm.request_move(src, dst_dir);
            }
            ImGui::EndDragDropTarget();
        }

        // ========================================================================
        // SAĞ TIK AÇILIR İÇERİK MENÜSÜ (CONTEXT MENU) - HER KLASÖR VE DOSYA İÇİN
        // ========================================================================
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            m_vm.select(path_str);
            m_context_target_path = entry.full_path;
            ImGui::OpenPopup("AssetItemContextMenu");
        }

        if (ImGui::BeginPopup("AssetItemContextMenu") || ImGui::BeginPopupContextItem("##AssetItemContextFallback")) {
            m_vm.select(path_str);
            m_context_target_path = entry.full_path;

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
                    project::engine_settings::instance().open_in_editor(path_str, project_root);
                }
                if (ImGui::MenuItem("Show in Explorer")) {
                    ShellExecuteW(NULL, L"open", L"explorer.exe", entry.full_path.c_str(), NULL, SW_SHOW);
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
                    project::engine_settings::instance().open_in_editor(path_str, project_root);
                }
                if (ImGui::MenuItem("Show in Explorer")) {
                    ShellExecuteW(NULL, L"open", L"explorer.exe", entry.full_path.parent_path().c_str(), NULL, SW_SHOW);
                }
                ImGui::Separator();
            }

            if (ImGui::MenuItem("Rename")) {
                m_context_target_path = entry.full_path;
                const std::string stem_u8 = (entry.type == asset_type::folder)
                    ? qual::path_to_utf8(entry.full_path.filename())
                    : qual::path_to_utf8(entry.full_path.stem());
                snprintf(m_rename_buffer, sizeof(m_rename_buffer), "%s", stem_u8.c_str());
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

        if (is_open && entry.type == asset_type::folder) {
            std::string qLower = to_lower_str(m_search_buffer);
            for (const auto& child : entry.children) {
                if (qLower.empty() || entry_matches_search(child, qLower)) {
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
