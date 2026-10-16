#pragma once

/**
 * @file top_bar_panel.h
 * @brief Editörün üst kontrol çubuğu (Top Bar) paneli için arayüz (View) katmanını tanımlar.
 */

#include "IEditorPanel.h"
#include "../ViewModels/top_bar_view_model.h"
#include <string>
#include <vector>

namespace vesta {

    /**
     * @class top_bar_panel
     * @brief Çalışma modu geçişlerini (Play/Stop), sahne dosya işlemlerini (Kaydet/Yükle) ve Window menüsünü çizen ImGui paneli.
     */
    class top_bar_panel : public IEditorPanel {
    public:
        /**
         * @brief Paneli ilgili görünüm modeli ve panel listesi referansı ile başlatır.
         * @param vm Panel eylemlerini ve durumunu yönetecek olan ViewModel referansı.
         * @param panels Sistemdeki tüm panellerin listesi (Window menüsünden açmak için).
         */
        explicit top_bar_panel(top_bar_view_model& vm, const std::vector<IEditorPanel*>* panels = nullptr);
        ~top_bar_panel() override = default;

        void set_panels_reference(const std::vector<IEditorPanel*>* panels) { m_panels = panels; }

        // ==========================================
        // IEditorPanel ARAYÜZÜ (INTERFACE)
        // ==========================================

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }

    private:
        IEditorPanel* find_panel(const std::string& title) const;

    private:
        top_bar_view_model& m_vm;
        const std::vector<IEditorPanel*>* m_panels = nullptr;
        std::string m_title = "TopBar";
        bool m_is_open = true;
    };

} // namespace vesta