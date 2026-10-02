/**
 * @file asset_browser_view_model.cpp
 * @brief Varlık Tarayıcısı (Asset Browser) görünüm modelinin mantıksal uygulaması.
 */

#include "asset_browser_view_model.h"
#include "../Services/asset_browser_system.h"
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
        // Eğer hedeflenen yol bir klasör değilse, bir üst dizinini (parent) baz al
        std::filesystem::path dir = std::filesystem::is_directory(target_dir)
            ? target_dir
            : target_dir.parent_path();

        // İsim çakışmalarını önlemek için sonuna numara ekle (örnek: YeniMateryal_1)
        std::string unique_name = name;
        int counter = 1;
        while (std::filesystem::exists(dir / (unique_name + ".json"))) {
            unique_name = name + "_" + std::to_string(counter++);
        }

        if (m_browser_system.create_material(dir, unique_name)) {
            refresh();
        }
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