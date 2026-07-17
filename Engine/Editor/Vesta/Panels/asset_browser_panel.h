#pragma once
#include <filesystem>
#include "../Services/asset_entry.h"
#include "IEditorPanel.h"

namespace vesta {

    class asset_browser_view_model;

    class asset_browser_panel : public vesta::IEditorPanel {
    public:
        explicit asset_browser_panel(asset_browser_view_model& vm);

        void init();

        // --- vesta::IEditorPanel ---
        void render() override;
        const std::string& get_title() const override;
        bool is_open() const override;
        void set_open(bool open) override;

    private:
        void draw_node(const asset_file_entry& entry);

        asset_browser_view_model& m_vm;

        std::string m_title = "Asset Browser";
        bool m_is_open = true;

        // Sadece bu panel'e ait geçici UI state
        std::filesystem::path m_context_target_path;
        char m_rename_buffer[128]{};
        char m_create_buffer[128] = "NewMaterial";
        bool m_open_rename_popup = false;
        bool m_open_create_popup = false;
    };

} // namespace vesta