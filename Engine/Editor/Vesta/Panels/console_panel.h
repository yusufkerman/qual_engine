#pragma once

/**
 * @file console_panel.h
 * @brief Vesta geliştirici konsol paneli (Developer Console Panel).
 * @details Merkezi logger'dan (engine_logger) thread-safe anlık görüntü (snapshot) alarak
 * seviye/kategori filtreleme, arama, log seli (spam) sayaçları, pano kopyalama ve
 * ilgili ECS varlığına tek tıkla odaklanma (jump-to-entity) özelliklerini sunar.
 */

#include "IEditorPanel.h"
#include "../../../Core/Logger/log_types.h"
#include <string>
#include <vector>

namespace vesta {

class editor_selection_manager;

/**
 * @class console_panel
 * @brief Motor, betik ve editör loglarını listeleyen ImGui konsol paneli.
 */
class console_panel : public IEditorPanel {
public:
    explicit console_panel(editor_selection_manager& selection_manager);
    ~console_panel() override = default;

    void render() override;
    [[nodiscard]] const std::string& get_title() const override { return m_title; }
    [[nodiscard]] bool is_open() const override { return m_is_open; }
    void set_open(bool open) override { m_is_open = open; }

private:
    void render_header_toolbar();
    void render_log_table();
    void render_detail_pane();

    [[nodiscard]] bool passes_filter(const qual::log::log_entry& entry) const;

private:
    editor_selection_manager& m_selection_manager;
    std::string m_title = "Console";
    bool m_is_open = true;

    // Arayüz Filtreleme Durumları (State)
    bool m_auto_scroll = true;
    bool m_show_info = true;
    bool m_show_warn = true;
    bool m_show_error = true;
    int m_category_filter = 0; // 0: Tümü, 1: Core, 2: Renderer, ...
    char m_search_buffer[128] = { 0 };

    // Seçili Log Satırı İnceleme Durumu
    int m_selected_entry_index = -1;

    // Thread-safe kopya tamponu (Veri yarışı önleyici yerel anlık görüntü)
    std::vector<qual::log::log_entry> m_snapshot;
};

} // namespace vesta

