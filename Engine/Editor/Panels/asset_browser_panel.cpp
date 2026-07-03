#include "asset_browser_panel.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>

namespace aria::editor {

    // Durum takibi için statik değişkenler
    static std::filesystem::path g_context_target_path = "";
    static char g_rename_buffer[128] = "";
    static char g_create_buffer[128] = "NewMaterial";
    static bool g_open_rename_popup = false;
    static bool g_open_create_popup = false;

    // 🚀 Gelen aria_sys pointer'ını m_browser_system kurucusuna güvenle paslıyoruz
    asset_browser_panel::asset_browser_panel(aria::system* aria_sys)
        : m_browser_system("Assets", aria_sys)
    {
    }

    void asset_browser_panel::init() {
        m_browser_system.refresh();
    }

    void asset_browser_panel::render(EditorContext& ctx) {
        ImGui::Begin("Asset Browser");

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            ctx.focusedPanel = FocusedPanel::AssetBrowser;
        }

        // 🔄 REFRESH BAR
        if (ImGui::Button("Refresh")) {
            m_browser_system.refresh();
        }

        ImGui::Separator();

        //  YENİ KOD (Sadece gerçekten boş alana tıklanırsa sıfırla):
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            // Eğer bir item'ın (ağaç düğümünün) üzerine tıklanmadıysa ve pencere boşluğuna tıklandıysa
            if (!ImGui::IsAnyItemHovered()) {
                m_selected_path.clear();
                ctx.selectedAssetPath.clear(); // Inspector'ı da temizle
            }
        }

        // ========================================================================
        // 🔥 BOŞ ALANA SAĞ TIKLANDIĞINDA (KÖK DİZİNE YENİ ASSET OLUŞTURMA)
        // ========================================================================
        if (ImGui::BeginPopupContextWindow("AssetBrowserContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("➕ Create Material...")) {
                g_context_target_path = "Assets"; // Kök dizin
                g_open_create_popup = true;
                snprintf(g_create_buffer, sizeof(g_create_buffer), "NewMaterial");
            }
            ImGui::EndPopup();
        }

        // 🔥 ROOT DÜĞÜMÜNÜ ÇİZ
        const auto& root = m_browser_system.root_entry();
        ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (m_selected_path == "Assets") root_flags |= ImGuiTreeNodeFlags_Selected;

        bool root_open = ImGui::TreeNodeEx("📁 Assets", root_flags);
        if (ImGui::IsItemClicked()) {
            m_selected_path = "Assets";
        }

        if (root_open) {
            for (const auto& child : root.children) {
                draw_node(child, ctx);
            }
            ImGui::TreePop();
        }

        // ========================================================================
        // 🛠️ MODAL POPUP: YENİ ASSET OLUŞTURMA (İSİM SORARAK)
        // ========================================================================
        if (g_open_create_popup) {
            ImGui::OpenPopup("Create Asset Name");
            g_open_create_popup = false;
        }

        if (ImGui::BeginPopupModal("Create Asset Name", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter material name:");
            ImGui::InputText("##create_name", g_create_buffer, sizeof(g_create_buffer));
            ImGui::Spacing();

            if (ImGui::Button("Create", ImVec2(120, 0))) {

                std::filesystem::path target_dir = g_context_target_path;

                if (!std::filesystem::is_directory(target_dir))
                    target_dir = target_dir.parent_path();

                std::string base_name = g_create_buffer;
                std::string unique_name = base_name;

                int counter = 1;
                while (std::filesystem::exists(target_dir / (unique_name + ".json"))) {
                    unique_name = base_name + "_" + std::to_string(counter++);
                }

                if (m_browser_system.create_material(target_dir, unique_name)) {
                    m_browser_system.refresh();
                }

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
        if (g_open_rename_popup) {
            ImGui::OpenPopup("Rename Asset");
            g_open_rename_popup = false;
        }

        if (ImGui::BeginPopupModal("Rename Asset", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Rename '%s' to:", g_context_target_path.filename().string().c_str());
            ImGui::InputText("##rename_input", g_rename_buffer, sizeof(g_rename_buffer));
            ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0))) {

                if (m_browser_system.rename_asset(g_context_target_path, g_rename_buffer)) {

                    std::filesystem::path new_path =
                        g_context_target_path.parent_path() /
                        (std::string(g_rename_buffer) + g_context_target_path.extension().string());

                    m_browser_system.refresh();

                    if (m_selected_path == g_context_target_path.string()) {
                        m_selected_path = new_path.string();
                    }
                }

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

    void asset_browser_panel::draw_node(const asset_file_entry& entry, EditorContext& ctx) {
        std::string path_str = entry.full_path.string();
        ImGui::PushID(path_str.c_str());

        std::string prefix = (entry.type == asset_type::folder) ? "📁 " : "📄 ";
        std::string label = prefix + entry.name;
        bool is_selected = (m_selected_path == path_str);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (entry.type != asset_type::folder) flags |= ImGuiTreeNodeFlags_Leaf;
        if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

        // ImGui'nin ağaç yapısını çiziyoruz
        bool is_open = ImGui::TreeNodeEx(label.c_str(), flags);

        // ========================================================================
        // 🚀 GÜVENLİ ÇİFT TIKLAMA SEÇİM MANTIĞI
        // ========================================================================
        if (ImGui::IsItemHovered()) {
            // Sadece sol fare tuşu ile ÇİFT tıklandığında Inspector'ı tetikle
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_selected_path = path_str;
                ctx.selectedAssetPath = path_str; // 🎯 Inspector paneli şimdi açılır!

                if (ctx.editor) {
                    ctx.editor->setSelectedEntity(-1);
                }
            }

            // Olası bir sağ tık durumunda (Context Menu için) o ögeyi arka planda 
            // seçili bırakmak isteyebilirsin, istersen burası aynen kalabilir:
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                m_selected_path = path_str;
            }
        }

        // ========================================================================
        // 🔥 ÖĞEYE ÖZEL SAĞ TIK MENÜSÜ (CONTEXT MENU)
        // ========================================================================
        if (ImGui::BeginPopupContextItem("ItemContextMenu")) {
            m_selected_path = path_str;
            ctx.selectedAssetPath = path_str;

            if (entry.type == asset_type::folder) {
                if (ImGui::MenuItem("➕ Create Material Here...")) {
                    g_context_target_path = entry.full_path;
                    g_open_create_popup = true;
                    snprintf(g_create_buffer, sizeof(g_create_buffer), "NewMaterial");
                }
                ImGui::Separator();
            }

            if (ImGui::MenuItem("📝 Rename")) {
                g_context_target_path = entry.full_path;
                snprintf(g_rename_buffer, sizeof(g_rename_buffer), "%s", entry.full_path.stem().string().c_str());
                g_open_rename_popup = true;
            }

            if (ImGui::MenuItem("👯 Duplicate / Copy")) {
                if (m_browser_system.duplicate_asset(entry.full_path)) {
                    m_browser_system.refresh();
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("🗑️ Delete")) {
                if (m_browser_system.delete_asset(entry.full_path)) {
                    m_browser_system.refresh();
                    if (m_selected_path == path_str) {
                        m_selected_path.clear();
                        ctx.selectedAssetPath.clear();
                    }
                }
            }

            ImGui::EndPopup();
        }

        // ========================================================================
        // 🚀 SÜREKLEME KAYNAĞI (DRAG SOURCE)
        // ========================================================================
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            aria::editor::asset_drag_payload payload{};
            std::string ext = entry.full_path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // Küçük harfe zorla

            if (ext == ".json") {
                payload.type = aria::editor::asset_payload_type::material;
            }
            else if (ext == ".png" || ext == ".jpg" || ext == ".tga") {
                payload.type = aria::editor::asset_payload_type::texture;
            }
            else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf") {
                payload.type = aria::editor::asset_payload_type::model;
            }
            else {
                payload.type = aria::editor::asset_payload_type::model; // Fallback
            }

            snprintf(payload.path, sizeof(payload.path), "%s", path_str.c_str());
            payload.guid = entry.guid;

            ImGui::SetDragDropPayload("ARIA_ASSET_PAYLOAD", &payload, sizeof(payload));

            // Görsel geribildirim
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
                    auto* data = (const aria::editor::asset_drag_payload*)imgui_payload->Data;
                    std::filesystem::path src(data->path);
                    std::filesystem::path dst = entry.full_path / src.filename();

                    try {
                        if (src != dst && std::filesystem::exists(src)) {
                            std::filesystem::rename(src, dst);
                            m_browser_system.refresh();
                        }
                    }
                    catch (const std::exception& e) {
                        std::cout << "❌ [Asset Browser] Tasima hatasi: " << e.what() << std::endl;
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        if (is_open) {
            if (entry.type == asset_type::folder) {
                for (const auto& child : entry.children) {
                    draw_node(child, ctx);
                }
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

} // namespace aria::editor