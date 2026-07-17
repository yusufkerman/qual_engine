// asset_browser_view_model.h
#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include "../Services/asset_entry.h"

namespace vesta {

    class asset_browser_system;
    class material_service;

    class asset_browser_view_model {
    public:
        asset_browser_view_model(
            asset_browser_system& browser_system,
            material_service& mat_service,
            std::function<void(const std::string&)> on_asset_selected,
            std::function<void()> on_entity_deselect_requested
        );

        const asset_file_entry& root() const;
        const std::string& selected_path() const { return m_selected_path; }
        bool is_selected(const std::string& path) const { return m_selected_path == path; }

        void refresh();
        void select(const std::string& path);
        void select_and_notify(const std::string& path);
        void clear_selection();

        void request_create_material(const std::filesystem::path& target_dir, const std::string& name);
        void request_rename(const std::filesystem::path& target, const std::string& new_name);
        void request_duplicate(const std::filesystem::path& target);
        void request_delete(const std::filesystem::path& target);
        void request_move(const std::filesystem::path& src, const std::filesystem::path& dst_dir);

    private:
        asset_browser_system& m_browser_system;
        material_service& m_material_service;

        std::function<void(const std::string&)> m_on_asset_selected;
        std::function<void()> m_on_entity_deselect_requested;

        std::string m_selected_path;
    };

} // namespace vesta