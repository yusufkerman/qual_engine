/**
 * @file console_panel.cpp
 * @brief Vesta geliştirici konsol paneli (Developer Console Panel) arayüz uygulaması.
 */

#include "console_panel.h"
#include "../../../Core/Logger/logger.h"
#include "../CoreState/editor_selection_manager.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <format>
#include <string>

namespace vesta {

console_panel::console_panel(editor_selection_manager& selection_manager)
    : m_selection_manager(selection_manager)
{
}

void console_panel::render()
{
    if (!m_is_open) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(700, 360), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(m_title.c_str(), &m_is_open)) {
        ImGui::End();
        return;
    }

    // Thread-safe kilit altında motor halka tamponunun güvenli anlık görüntüsünü al
    qual::log::engine_logger::instance().get_snapshot(m_snapshot);

    render_header_toolbar();
    ImGui::Separator();
    render_log_table();
    render_detail_pane();

    ImGui::End();
}

void console_panel::render_header_toolbar()
{
    // İstatistik ve sayaç hesaplaması
    size_t info_count = 0;
    size_t warn_count = 0;
    size_t error_count = 0;
    for (const auto& e : m_snapshot) {
        if (e.level <= qual::log::log_level::info) {
            ++info_count;
        } else if (e.level == qual::log::log_level::warn) {
            ++warn_count;
        } else {
            ++error_count;
        }
    }

    // 1. Temizleme Butonu
    if (ImGui::Button("Clear")) {
        qual::log::engine_logger::instance().clear();
        m_snapshot.clear();
        m_selected_entry_index = -1;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all logs from the engine ring buffer");
    }

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_auto_scroll);

    ImGui::SameLine();
    if (ImGui::Button("Copy All")) {
        std::string all_text;
        all_text.reserve(m_snapshot.size() * 128);
        for (const auto& e : m_snapshot) {
            if (!passes_filter(e)) continue;
            all_text += std::format("[{}] [{}] {} ({}:{})",
                to_string(e.level), to_string(e.category), e.message, e.file, e.line);
            if (e.entity_id != 0) {
                all_text += std::format(" [Entity #{}]", e.entity_id);
            }
            if (e.repeat_count > 1) {
                all_text += std::format(" (x{})", e.repeat_count);
            }
            all_text += "\n";
        }
        ImGui::SetClipboardText(all_text.c_str());
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Copy all filtered logs to the system clipboard");
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // 2. Kategori Filtresi
    const char* categories[] = {
        "All Categories", "Core", "Renderer", "Physics",
        "Audio", "Script", "Asset", "Editor", "System"
    };
    ImGui::SetNextItemWidth(120.0f);
    ImGui::Combo("##CategoryFilter", &m_category_filter, categories, IM_ARRAYSIZE(categories));

    ImGui::SameLine();

    // 3. Metin Arama Kutusu
    ImGui::SetNextItemWidth(140.0f);
    ImGui::InputTextWithHint("##SearchFilter", "Search logs...", m_search_buffer, sizeof(m_search_buffer));

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // 4. Seviye Filtreleme Butonları (Rozetli)
    char info_label[32];
    std::snprintf(info_label, sizeof(info_label), "Info: %zu", info_count);
    if (m_show_info) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.7f, 0.85f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.4f));
    }
    if (ImGui::Button(info_label)) {
        m_show_info = !m_show_info;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    char warn_label[32];
    std::snprintf(warn_label, sizeof(warn_label), "Warn: %zu", warn_count);
    if (m_show_warn) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.55f, 0.1f, 0.85f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.4f));
    }
    if (ImGui::Button(warn_label)) {
        m_show_warn = !m_show_warn;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    char error_label[32];
    std::snprintf(error_label, sizeof(error_label), "Error: %zu", error_count);
    if (m_show_error) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.2f, 0.2f, 0.85f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.4f));
    }
    if (ImGui::Button(error_label)) {
        m_show_error = !m_show_error;
    }
    ImGui::PopStyleColor();
}

void console_panel::render_log_table()
{
    const float footer_height = (m_selected_entry_index >= 0) ? 130.0f : 0.0f;
    const ImVec2 table_size(0.0f, -footer_height);

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_BordersInnerH |
                                  ImGuiTableFlags_BordersOuter |
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_SizingFixedFit;

    if (ImGui::BeginTable("ConsoleLogTable", 6, flags, table_size)) {
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 85.0f);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Entity", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(m_snapshot.size()); ++i) {
            const auto& entry = m_snapshot[i];
            if (!passes_filter(entry)) {
                continue;
            }

            ImGui::TableNextRow();
            const bool is_selected = (m_selected_entry_index == i);

            // Kolon 0: Seviye
            ImGui::TableSetColumnIndex(0);
            ImVec4 level_col;
            switch (entry.level) {
                case qual::log::log_level::trace:
                case qual::log::log_level::debug:    level_col = ImVec4(0.5f, 0.8f, 1.0f, 1.0f); break;
                case qual::log::log_level::info:     level_col = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); break;
                case qual::log::log_level::warn:     level_col = ImVec4(1.0f, 0.85f, 0.25f, 1.0f); break;
                case qual::log::log_level::error:    level_col = ImVec4(1.0f, 0.35f, 0.35f, 1.0f); break;
                case qual::log::log_level::critical: level_col = ImVec4(1.0f, 0.15f, 0.45f, 1.0f); break;
                default:                             level_col = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;
            }

            char row_id[32];
            std::snprintf(row_id, sizeof(row_id), "##row_%d", i);
            if (ImGui::Selectable(row_id, is_selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
                m_selected_entry_index = is_selected ? -1 : i;
            }

            // Çift tıklandığında varlığa otomatik odaklan
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (entry.entity_id != 0) {
                    m_selection_manager.clear_all();
                    m_selection_manager.set_context(selection_context_type::scene_entities);
                    m_selection_manager.select_entity(entry.entity_id);
                }
            }

            // Sağ tık menüsü (Context Menu)
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Copy Message")) {
                    ImGui::SetClipboardText(entry.message);
                }
                if (entry.entity_id != 0 && ImGui::MenuItem("Select Associated Entity")) {
                    m_selection_manager.clear_all();
                    m_selection_manager.set_context(selection_context_type::scene_entities);
                    m_selection_manager.select_entity(entry.entity_id);
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            ImGui::TextColored(level_col, "%s", to_string(entry.level));

            // Kolon 1: Zaman (HH:MM:SS.mmm)
            ImGui::TableSetColumnIndex(1);
            const uint64_t total_sec = entry.timestamp_ms / 1000;
            const uint32_t ms = static_cast<uint32_t>(entry.timestamp_ms % 1000);
            const uint32_t s = static_cast<uint32_t>(total_sec % 60);
            const uint32_t m = static_cast<uint32_t>((total_sec / 60) % 60);
            const uint32_t h = static_cast<uint32_t>((total_sec / 3600) % 24);
            ImGui::TextDisabled("%02u:%02u:%02u.%03u", h, m, s, ms);

            // Kolon 2: Kategori
            ImGui::TableSetColumnIndex(2);
            if (entry.category == qual::log::log_category::script) {
                ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f), "[%s]", to_string(entry.category));
            } else {
                ImGui::TextDisabled("[%s]", to_string(entry.category));
            }

            // Kolon 3: Mesaj & Tekrar Rozeti (Spam Dedup Badge)
            ImGui::TableSetColumnIndex(3);
            if (entry.repeat_count > 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.1f, 1.0f), "[x%u]", entry.repeat_count);
                ImGui::SameLine();
            }
            ImGui::TextColored(level_col, "%s", entry.message);

            // Kolon 4: Dosya ve Satır
            ImGui::TableSetColumnIndex(4);
            ImGui::TextDisabled("%s:%u", entry.file, entry.line);

            // Kolon 5: Varlık (Entity)
            ImGui::TableSetColumnIndex(5);
            if (entry.entity_id != 0) {
                char btn_id[32];
                std::snprintf(btn_id, sizeof(btn_id), "Entity #%u##e_%d", entry.entity_id, i);
                if (ImGui::SmallButton(btn_id)) {
                    m_selection_manager.clear_all();
                    m_selection_manager.set_context(selection_context_type::scene_entities);
                    m_selection_manager.select_entity(entry.entity_id);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Click to select this entity in Hierarchy and Inspector");
                }
            } else {
                ImGui::TextDisabled("-");
            }
        }

        // Otomatik kaydırma
        if (m_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
    }
}

void console_panel::render_detail_pane()
{
    if (m_selected_entry_index < 0 || m_selected_entry_index >= static_cast<int>(m_snapshot.size())) {
        return;
    }

    const auto& entry = m_snapshot[m_selected_entry_index];

    ImGui::Separator();
    ImGui::BeginChild("LogDetailPane", ImVec2(0, 120.0f), false);

    ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Log Details:");
    ImGui::SameLine();
    ImGui::TextDisabled("| %s | %s | %s:%u", to_string(entry.level), to_string(entry.category), entry.file, entry.line);

    if (entry.entity_id != 0) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.35f, 0.92f, 0.52f, 1.0f), "| Entity: #%u", entry.entity_id);
        ImGui::SameLine();
        if (ImGui::SmallButton("Select In Editor")) {
            m_selection_manager.clear_all();
            m_selection_manager.set_context(selection_context_type::scene_entities);
            m_selection_manager.select_entity(entry.entity_id);
        }
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Copy Message")) {
        ImGui::SetClipboardText(entry.message);
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("Close [X]")) {
        m_selected_entry_index = -1;
    }

    // Ayrıntılı mesaj içeriğini salt-okunur çok satırlı metin kutusunda göster
    char detail_buf[512];
    std::snprintf(detail_buf, sizeof(detail_buf), "%s", entry.message);
    ImGui::InputTextMultiline("##DetailText", detail_buf, sizeof(detail_buf),
                              ImVec2(-1, 80.0f), ImGuiInputTextFlags_ReadOnly);

    ImGui::EndChild();
}

bool console_panel::passes_filter(const qual::log::log_entry& entry) const
{
    // 1. Seviye Kontrolü
    if (entry.level <= qual::log::log_level::info && !m_show_info) return false;
    if (entry.level == qual::log::log_level::warn && !m_show_warn) return false;
    if (entry.level >= qual::log::log_level::error && !m_show_error) return false;

    // 2. Kategori Kontrolü (0: Tümü)
    if (m_category_filter > 0) {
        const auto expected_cat = static_cast<qual::log::log_category>(m_category_filter - 1);
        if (entry.category != expected_cat) return false;
    }

    // 3. Metin Arama Kontrolü
    if (m_search_buffer[0] != '\0') {
        std::string query = m_search_buffer;
        std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        std::string msg = entry.message;
        std::transform(msg.begin(), msg.end(), msg.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        std::string file = entry.file;
        std::transform(file.begin(), file.end(), file.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (msg.find(query) == std::string::npos && file.find(query) == std::string::npos) {
            return false;
        }
    }

    return true;
}

} // namespace vesta
