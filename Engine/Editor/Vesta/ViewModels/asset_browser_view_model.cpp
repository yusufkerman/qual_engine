/**
 * @file asset_browser_view_model.cpp
 * @brief Varlık Tarayıcısı (Asset Browser) görünüm modelinin mantıksal uygulaması.
 */

#include "asset_browser_view_model.h"
#include "../../../Core/path_utils.h"
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
            project_assets_dir = qual::path_from_utf8(project::project_manager::instance().get_asset_path());
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
            select_and_notify(qual::path_to_utf8(new_mat_path));
        }
    }

    void asset_browser_view_model::request_create_script(
        const std::filesystem::path& target_dir, const std::string& script_name)
    {
        std::filesystem::path dir = target_dir;
        std::filesystem::path project_assets_dir;
        if (project::project_manager::instance().has_active_project()) {
            project_assets_dir = qual::path_from_utf8(project::project_manager::instance().get_asset_path());
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
            std::ofstream hFile(header_path, std::ios::out | std::ios::trunc);
            if (hFile.is_open()) {
                hFile << "#pragma once\n\n";
                hFile << "/**\n";
                hFile << " * @file " << unique_name << ".h\n";
                hFile << " * @brief Gameplay script for " << unique_name << ".\n";
                hFile << " */\n\n";
                hFile << "#include <Gameplay/gameplay.h>\n";
                hFile << "#include <Engine/Scripting/script_factory.h>\n";
                hFile << "#include <glm/glm.hpp>\n\n";
                hFile << "/**\n";
                hFile << " * @class " << unique_name << "\n";
                hFile << " * @brief Unity-style gameplay script component logic.\n";
                hFile << " */\n";
                hFile << "class " << unique_name << " : public gameplay::script\n";
                hFile << "{\n";
                hFile << "public:\n";
                hFile << "    // Start is called before the first frame update (Unity: Start)\n";
                hFile << "    void on_create() override;\n\n";
                hFile << "    // Update is called once per frame with delta time in seconds (Unity: Update)\n";
                hFile << "    void on_update(float dt) override;\n\n";
                hFile << "    // FixedUpdate is called on physics timesteps (Unity: FixedUpdate) - uncomment if needed\n";
                hFile << "    // void on_fixed_update(float fixed_dt) override;\n\n";
                hFile << "    // Called when this collider/rigidbody begins touching another (Unity: OnCollisionEnter)\n";
                hFile << "    // void on_collision_enter(const gameplay::collision_hit& hit) override;\n\n";
                hFile << "    // Called when a trigger sensor detects another entity (Unity: OnTriggerEnter)\n";
                hFile << "    // void on_trigger_enter(const gameplay::trigger_hit& hit) override;\n\n";
                hFile << "    // Called when the script or entity is being destroyed (Unity: OnDestroy)\n";
                hFile << "    // void on_destroy() override;\n\n";
                hFile << "private:\n";
                hFile << "    // Expose / define your script variables here\n";
                hFile << "    // float move_speed = 5.0f;\n";
                hFile << "};\n\n";
                hFile << "// Register script so it can be attached to entities in the Inspector\n";
                hFile << "REGISTER_SCRIPT(" << unique_name << ");\n";
                hFile.flush();
                hFile.close();
            }
        }

        {
            std::ofstream cppFile(source_path, std::ios::out | std::ios::trunc);
            if (cppFile.is_open()) {
                cppFile << "/**\n";
                cppFile << " * @file " << unique_name << ".cpp\n";
                cppFile << " * @brief Implementation for " << unique_name << ".\n";
                cppFile << " */\n\n";
                cppFile << "#include \"" << unique_name << ".h\"\n";
                cppFile << "#include <glm/glm.hpp>\n";
                cppFile << "#include <iostream>\n\n";
                cppFile << "// Start is called before the first frame update\n";
                cppFile << "void " << unique_name << "::on_create()\n";
                cppFile << "{\n";
                cppFile << "    // Use this for initialization (equivalent to Unity Start)\n";
                cppFile << "    // Example: Read or set initial transform position\n";
                cppFile << "    // glm::vec3 startPos = transform().get_position();\n";
                cppFile << "    // gameplay::log::info(\"" << unique_name << " initialized on entity: \" + std::to_string(entity_id()));\n";
                cppFile << "}\n\n";
                cppFile << "// Update is called once per frame\n";
                cppFile << "void " << unique_name << "::on_update(float dt)\n";
                cppFile << "{\n";
                cppFile << "    // Put per-frame logic here (equivalent to Unity Update)\n";
                cppFile << "    // Example: Input handling (WASD / Arrow Keys)\n";
                cppFile << "    // if (gameplay::input::is_key_down(GLFW_KEY_W)) {\n";
                cppFile << "    //     transform().translate(glm::vec3(0.0f, 0.0f, -5.0f * dt));\n";
                cppFile << "    // }\n";
                cppFile << "}\n";
                cppFile.flush();
                cppFile.close();
            }
        }

        refresh();
        const std::string header_u8 = qual::path_to_utf8(header_path);
        select_and_notify(header_u8);

        std::string project_root;
        if (project::project_manager::instance().has_active_project()) {
            project_root = project::project_manager::instance().get_project_root();
        }
        project::engine_settings::instance().open_in_editor(header_u8, project_root);
    }

    void asset_browser_view_model::request_create_folder(
        const std::filesystem::path& target_dir, const std::string& folder_name)
    {
        std::filesystem::path dir = target_dir;
        std::filesystem::path project_assets_dir;
        if (project::project_manager::instance().has_active_project()) {
            project_assets_dir = qual::path_from_utf8(project::project_manager::instance().get_asset_path());
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
        while (std::filesystem::exists(dir / qual::path_from_utf8(unique_name), ec)) {
            unique_name = clean_name + "_" + std::to_string(counter++);
        }

        std::filesystem::create_directories(dir / qual::path_from_utf8(unique_name), ec);
        refresh();
    }

    void asset_browser_view_model::request_rename(
        const std::filesystem::path& target, const std::string& new_name)
    {
        const bool is_dir = std::filesystem::is_directory(target);
        const std::string ext_u8 = is_dir ? std::string{} : qual::path_to_utf8(target.extension());
        const std::string target_u8 = qual::path_to_utf8(target);

        if (m_browser_system.rename_asset(target, new_name)) {
            std::string final_name_u8 = new_name;
            if (!is_dir && !ext_u8.empty() && !final_name_u8.ends_with(ext_u8)) {
                final_name_u8 += ext_u8;
            }
            std::filesystem::path new_path = target.parent_path() / qual::path_from_utf8(final_name_u8);

            refresh();

            // Eğer yeniden adlandırılan dosya şu an seçiliyse, seçim yolunu da güncelle
            if (m_selected_path == target_u8) {
                select(qual::path_to_utf8(new_path));
            }
        }
    }

    void asset_browser_view_model::request_duplicate(const std::filesystem::path& target) {
        if (m_browser_system.duplicate_asset(target)) {
            refresh();
        }
    }

    void asset_browser_view_model::request_delete(const std::filesystem::path& target) {
        const std::string target_u8 = qual::path_to_utf8(target);
        if (m_browser_system.delete_asset(target)) {
            if (m_selected_path == target_u8) {
                clear_selection();
            }
        }
    }

    void asset_browser_view_model::request_move(
        const std::filesystem::path& src, const std::filesystem::path& dst_dir)
    {
        try {
            const auto src_fs = qual::path_from_utf8(qual::path_to_utf8(src));
            auto dst_dir_fs = qual::path_from_utf8(qual::path_to_utf8(dst_dir));
            if (std::filesystem::exists(dst_dir_fs) && !std::filesystem::is_directory(dst_dir_fs)) {
                dst_dir_fs = dst_dir_fs.parent_path();
            }

            if (!std::filesystem::exists(src_fs) || !std::filesystem::exists(dst_dir_fs)) {
                return;
            }

            std::string src_can = qual::path_to_utf8(std::filesystem::weakly_canonical(src_fs));
            std::string dst_can = qual::path_to_utf8(std::filesystem::weakly_canonical(dst_dir_fs));
            if (dst_can.rfind(src_can, 0) == 0) {
                std::cout << "[Vesta AssetBrowser] Uyari: Bir klasor kendi icine tasinamaz!\n";
                return;
            }

            std::filesystem::path dst_file_fs = dst_dir_fs / src_fs.filename();
            if (src_fs == dst_file_fs) {
                return;
            }

            std::filesystem::rename(src_fs, dst_file_fs);

            std::filesystem::path src_meta = qual::path_from_utf8(qual::path_to_utf8(src_fs) + ".meta");
            std::filesystem::path dst_meta = qual::path_from_utf8(qual::path_to_utf8(dst_file_fs) + ".meta");
            if (std::filesystem::exists(src_meta)) {
                std::filesystem::rename(src_meta, dst_meta);
            }

            refresh();
            std::cout << "[Vesta AssetBrowser] Tasindi: " << qual::path_to_utf8(src_fs.filename()) << " -> " << qual::path_to_utf8(dst_dir_fs) << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "[Vesta AssetBrowser] Hata: Dosya tasinirken bir sorun olustu: " << e.what() << "\n";
        }
    }

} // namespace vesta
