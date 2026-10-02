#pragma once

/**
 * @file top_bar_panel.h
 * @brief Editörün üst kontrol çubuğu (Top Bar) paneli için arayüz (View) katmanını tanımlar.
 */

#include "IEditorPanel.h"
#include "../ViewModels/top_bar_view_model.h"
#include <string>

namespace vesta {

    /**
     * @class top_bar_panel
     * @brief Çalışma modu geçişlerini (Play/Stop) ve sahne dosya işlemlerini (Kaydet/Yükle) çizen ImGui paneli.
     */
    class top_bar_panel : public IEditorPanel {
    public:
        /**
         * @brief Paneli ilgili görünüm modeli (ViewModel) ile başlatır.
         * @param vm Panel eylemlerini ve durumunu yönetecek olan ViewModel referansı.
         */
        explicit top_bar_panel(top_bar_view_model& vm);
        ~top_bar_panel() override = default;

        // ==========================================
        // IEditorPanel ARAYÜZÜ (INTERFACE)
        // ==========================================

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }

    private:
        top_bar_view_model& m_vm;
        std::string m_title = "TopBar";
        bool m_is_open = true;
    };

} // namespace vesta