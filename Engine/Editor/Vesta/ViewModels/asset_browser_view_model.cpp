/**
 * @file asset_browser_view_model.cpp
 * @brief Varlık Tarayıcısı (Asset Browser) görünüm modelinin mantıksal uygulaması.
 */

#include "asset_browser_view_model.h"
#include "../Services/asset_browser_system.h"
#include "../../Project/project.h"
#include "../../Project/engine_settings.h"
#include <fstream>
#include <iostream>

namespace vesta {

    asset_browser_view_model::asset_browser_view_model(
        asset_browser_system& browser_system,
        material_service& mat_service,
        std::function<void(const std::string&)> on_asset_selected,
        std::function<void()> on_entity_deselect_requested)
        : m_browser_system(browser_system)
        , m_material_service(mat_service)
        , m_on_asset_selected(std::move(on_asset_selected))
        , m_on_entity_deselect_requested(std::move(on_entity_deselect_requested))
    {
    }

    const asset_file_entry& asset_browser_view_model::root() const {
        return m_browser_system.root_entry();
    }

    void asset_browser_view_model::refresh() {
        m_browser_system.refresh();
    }

    void asset_browser_view_model::select(const std::string& path) {
        m_selected_path = path;
    }

    void asset_browser_view_model::select_and_notify(const std::string& path) {
        m_selected_path = path;

        if (m_on_asset_selected) {
            m_on_asset_selected(path);
        }

        // Bir varlık (asset) seçildiğinde sahnedeki aktif nesne seçimini kaldır.
        if (m_on_entity_deselect_requested) {
            m_on_entity_deselect_requested();
        }
    }

    void asset_browser_view_model::clear_selection() {
        m_selected_path.clear();
        if (m_on_asset_selected) {
            m_on_asset_selected("");
        }
    }

    void asset_browser_view_model::request_create_material(
        const std::filesystem::path& target_dir, const std::string& name)
    {
        std::filesystem::path dir = target_dir;

        // Açık olan projenin gerçek varlık yolu (varsa)
        std::filesystem::path project_assets_dir;
        if (project::project_manager::instance().has_active_project()) {
            project_assets_dir = project::project_manager::instance().get_asset_path();
        }
        if (project_assets_dir.empty()) {
            project_assets_dir = m_browser_system.root_entry().full_path;
        }

        // Hedef dizin boşsa doğrudan açık projenin / browser'ın kök dizinini kullan
        if (dir.empty()) {
            dir = project_assets_dir;
        }
        // Eğer hedef dizin mevcut bir dosya ise, onun üst klasörünü kullan
        else if (std::filesystem::exists(dir) && !std::filesystem::is_directory(dir)) {
            dir = dir.parent_path();
        }
        // Eğer hedef dizin mevcut değilse veya göreceliyse
        else if (!std::filesystem::exists(dir)) {
            if (dir == "Assets" || dir == m_browser_system.root_entry().name) {
                dir = project_assets_dir;
            } else {
                std::filesystem::path combined = project_assets_dir / dir;
                if (std::filesystem::exists(combined)) {
                    dir = std::filesystem::is_directory(combined) ? combined : combined.parent_path();
                } else {
                    dir = project_assets_dir;
                }
            }
        }

        // Klasörün var olduğundan emin ol
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) {
            std::filesystem::create_directories(dir, ec);
        }

        // İsim temizleme ve varsayılan atama
        std::string clean_name = name;
        if (clean_name.empty()) {
            clean_name = "NewMaterial";
        }
        if (clean_name.ends_with(".json")) {
            clean_name = clean_name.substr(0, clean_name.length() - 5);
        } else if (clean_name.ends_with(".amat")) {
            clean_name = clean_name.substr(0, clean_name.length() - 5);
        }

        // İsim çakışmalarını önlemek için sonuna numara ekle (örnek: NewMaterial_1)
        std::string unique_name = clean_name;
        int counter = 1;
        while (std::filesystem::exists(dir / (unique_name + ".json"), ec)) {
            unique_name = clean_name + "_" + std::to_string(counter++);
        }

        if (m_browser_system.create_material(dir, unique_name)) {
            refresh();
            std::filesystem::path new_mat_path = dir / (unique_name + ".json");
            select_and_notify(new_mat_path.string());
        }
    }

    void asset_browser_view_model::request_create_script(
        const std::filesystem::path& target_dir, const std::string& script_name)
    {
        std::filesystem::path dir = target_dir;
        std::filesystem::path project_assets_dir;
        if (project::project_manager::instance().has_active_project()) {
            project_assets_dir = project::project_manager::instance().get_asset_path();
        }
        if (project_assets_dir.empty()) {
            project_assets_dir = m_browser_system.root_entry().full_path;
        }

        if (dir.empty()) {
            dir = project_assets_dir;
        }
        else if (std::filesystem::exists(dir) && !std::filesystem::is_directory(dir)) {
            dir = dir.parent_path();
        }
        else if (!std::filesystem::exists(dir)) {
            std::filesystem::path combined = project_assets_dir / dir;
            if (std::filesystem::exists(combined)) {
                dir = std::filesystem::is_directory(combined) ? combined : combined.parent_path();
            } else {
                dir = project_assets_dir;
            }
        }

        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) {
            std::filesystem::create_directories(dir, ec);
        }

        std::string clean_name = script_name;
        if (clean_name.empty()) {
            clean_name = "PlayerController";
        }
        if (clean_name.ends_with(".h")) {
            clean_name = clean_name.substr(0, clean_name.length() - 2);
        } else if (clean_name.ends_with(".cpp")) {
            clean_name = clean_name.substr(0, clean_name.length() - 4);
        }

        for (char& c : clean_name) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
                c = '_';
            }
        }
        if (!clean_name.empty() && std::isdigit(static_cast<unsigned char>(clean_name[0]))) {
            clean_name = "_" + clean_name;
        }

        std::string unique_name = clean_name;
        int counter = 1;
        while (std::filesystem::exists(dir / (unique_name + ".h"), ec) ||
               std::filesystem::exists(dir / (unique_name + ".cpp"), ec)) {
            unique_name = clean_name + "_" + std::to_string(counter++);
        }

        std::filesystem::path header_path = dir / (unique_name + ".h");
        std::filesystem::path source_path = dir / (unique_name + ".cpp");

        {
            std::ofstream hFile(header_path);
            if (hFile.is_open()) {
                hFile << "#pragma once\n\n";
                hFile << "/**\n";
                hFile << " * @file " << unique_name << ".h\n";
                hFile << " * @brief Gameplay API script for " << unique_name << ".\n";
                hFile << " */\n\n";
                hFile << "#include <Gameplay/gameplay.h>\n";
                hFile << "#include <Engine/Scripting/script_factory.h>\n";
                hFile << "#include <glm/glm.hpp>\n\n";
                hFile << "/**\n";
                hFile << " * @class " << unique_name << "\n";
                hFile << " * @brief Script component logic.\n";
                hFile << " */\n";
                hFile << "class " << unique_name << " : public gameplay::script\n";
                hFile << "{\n";
                hFile << "public:\n";
                hFile << "    void on_create() override;\n";
                hFile << "    void on_update(float dt) override;\n";
                hFile << "};\n\n";
                hFile << "REGISTER_SCRIPT(" << unique_name << ");\n";
            }
        }

        {
            std::ofstream cppFile(source_path);
            if (cppFile.is_open()) {
                cppFile << "/**\n";
                cppFile << " * @file " << unique_name << ".cpp\n";
                cppFile << " * @brief Implementation for " << unique_name << ".\n";
                cppFile << " */\n\n";
                cppFile << "#include \"" << unique_name << ".h\"\n";
                cppFile << "#include <glm/glm.hpp>\n\n";
                cppFile << "void " << unique_name << "::on_create()\n";
                cppFile << "{\n";
                cppFile << "    // Initialize entity and components\n";
                cppFile << "}\n\n";
                cppFile << "void " << unique_name << "::on_update(float dt)\n";
                cppFile << "{\n";
                cppFile << "    // Per-frame logic here\n";
                cppFile << "}\n";
            }
        }

        refresh();
        select_and_notify(header_path.string());

        std::string project_root;
        if (project::project_manager::instance().has_active_project()) {
            project_root = project::project_manager::instance().get_project_root();
        }
        project::engine_settings::instance().open_in_editor(header_path.string(), project_root);
    }

    void asset_browser_view_model::request_create_folder(
        const std::filesystem::path& target_dir, const std::string& folder_name)
    {
        std::filesystem::path dir = target_dir;
        std::filesystem::path project_assets_dir;
        if (project::project_manager::instance().has_active_project()) {
            project_assets_dir = project::project_manager::instance().get_asset_path();
        }
        if (project_assets_dir.empty()) {
            project_assets_dir = m_browser_system.root_entry().full_path;
        }

        if (dir.empty()) {
            dir = project_assets_dir;
        }
        else if (std::filesystem::exists(dir) && !std::filesystem::is_directory(dir)) {
            dir = dir.parent_path();
        }
        else if (!std::filesystem::exists(dir)) {
            std::filesystem::path combined = project_assets_dir / dir;
            if (std::filesystem::exists(combined)) {
                dir = std::filesystem::is_directory(combined) ? combined : combined.parent_path();
            } else {
                dir = project_assets_dir;
            }
        }

        std::string clean_name = folder_name.empty() ? "NewFolder" : folder_name;
        std::error_code ec;
        std::string unique_name = clean_name;
        int counter = 1;
        while (std::filesystem::exists(dir / unique_name, ec)) {
            unique_name = clean_name + "_" + std::to_string(counter++);
        }

        std::filesystem::create_directories(dir / unique_name, ec);
        refresh();
    }

    void asset_browser_view_model::request_rename(
        const std::filesystem::path& target, const std::string& new_name)
    {
        if (m_browser_system.rename_asset(target, new_name)) {
            std::filesystem::path new_path =
                target.parent_path() / (new_name + target.extension().string());

            refresh();

            // Eğer yeniden adlandırılan dosya şu an seçiliyse, seçim yolunu da güncelle
            if (m_selected_path == target.string()) {
                select(new_path.string());
            }
        }
    }

    void asset_browser_view_model::request_duplicate(const std::filesystem::path& target) {
        if (m_browser_system.duplicate_asset(target)) {
            refresh();
        }
    }

    void asset_browser_view_model::request_delete(const std::filesystem::path& target) {
        // Not: İleride Undo/Redo mekanizması için Command Stack'e entegre edilebilir.
        if (m_browser_system.delete_asset(target)) {
            if (m_selected_path == target.string()) {
                clear_selection();
            }
        }
    }

    void asset_browser_view_model::request_move(
        const std::filesystem::path& src, const std::filesystem::path& dst_dir)
    {
        try {
            std::filesystem::path dst = dst_dir / src.filename();
            if (src != dst && std::filesystem::exists(src)) {
                std::filesystem::rename(src, dst);
                refresh();
            }
        }
        catch (const std::exception& e) {
            // Sessizce yutulan hata düzeltildi. Dosya kilidi (lock) veya izin sorunlarında uyarır.
            std::cout << "[Vesta AssetBrowser] Hata: Dosya tasinirken bir sorun olustu: " << e.what() << "\n";
        }
    }

} // namespace vesta