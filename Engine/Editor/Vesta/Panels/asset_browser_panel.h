#pragma once

/**
 * @file asset_browser_panel.h
 * @brief Varlık Tarayıcısı (Asset Browser) paneli için kullanıcı arayüzü (View) katmanını tanımlar.
 */

#include <filesystem>
#include "../Services/asset_entry.h"
#include "IEditorPanel.h"

namespace vesta {

    class asset_browser_view_model;

    /**
     * @class asset_browser_panel
     * @brief Dosya sistemini hiyerarşik bir ağaç yapısında çizen, sağ tık menüleri ve sürükle-bırak işlemlerini yöneten ImGui paneli.
     * @note İş mantığı barındırmaz, kullanıcı eylemlerini asset_browser_view_model'e (MVVM) iletir.
     */
    class asset_browser_panel : public vesta::IEditorPanel {
    public:
        /**
         * @brief Paneli ilgili görünüm modeli (ViewModel) ile başlatır.
         * @param vm UI ile arka plan servisleri arasında köprü kuracak olan ViewModel.
         */
        explicit asset_browser_panel(asset_browser_view_model& vm);

        /**
         * @brief Panelin iç durumunu ilk kullanıma hazırlar ve dosya ağacını yeniler.
         */
        void init();

        // ==========================================
        // IEditorPanel ARAYÜZÜ (INTERFACE)
        // ==========================================

        void render() override;
        const std::string& get_title() const override;
        bool is_open() const override;
        void set_open(bool open) override;

    private:
        /**
         * @brief Dosya veya klasör düğümünü (node) ağaç yapısında özyinelemeli (recursive) olarak çizer.
         * @param entry Çizilecek olan dosya/klasör girdisi.
         */
        void draw_node(const asset_file_entry& entry);

    private:
        asset_browser_view_model& m_vm;

        std::string m_title = "Asset Browser";
        bool m_is_open = true;

        // --- PANEL'E ÖZEL GEÇİCİ UI DURUM (STATE) DEĞİŞKENLERİ ---

        std::filesystem::path m_context_target_path;    ///< Üzerinde işlem yapılan geçerli dosya/klasör yolu.
        char m_rename_buffer[128]{};                    ///< Yeniden adlandırma popup'ı için metin tamponu.
        char m_create_buffer[128] = "NewMaterial";      ///< Yeni varlık oluşturma popup'ı için metin tamponu.
        char m_create_script_buffer[128] = "PlayerController";
        char m_create_folder_buffer[128] = "NewFolder";

        bool m_open_rename_popup = false;               ///< Yeniden adlandırma penceresini tetikleme bayrağı.
        bool m_open_create_popup = false;               ///< Yeni materyal oluşturma penceresini tetikleme bayrağı.
        bool m_open_create_script_popup = false;        ///< Yeni betik (C++ script) oluşturma bayrağı.
        bool m_open_create_folder_popup = false;        ///< Yeni klasör oluşturma bayrağı.
    };

} // namespace vesta