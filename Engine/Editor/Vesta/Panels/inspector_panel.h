#pragma once

/**
 * @file inspector_panel.h
 * @brief Inspector (Özellikler/Denetçi) paneli için kullanıcı arayüzü (View) katmanını tanımlar.
 */

#include "IEditorPanel.h"
#include "../ViewModels/inspector_view_model.h"
#include <memory>
#include <string>

namespace vesta
{
    /**
     * @class inspector_panel
     * @brief Seçili varlığın (Entity) bileşenlerini veya seçili dosyanın (Asset) özelliklerini çizen ImGui paneli.
     */
    class inspector_panel final : public IEditorPanel
    {
    public:
        /**
         * @brief Inspector panelini görünüm modeli (ViewModel) sahipliği ile başlatır.
         * @param vm Panel mantığını ve veri akışını yönetecek olan ViewModel.
         */
        explicit inspector_panel(std::unique_ptr<inspector_view_model> vm);
        ~inspector_panel() override;

        // ==========================================
        // IEditorPanel ARAYÜZÜ (INTERFACE)
        // ==========================================

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }

    private:
        std::unique_ptr<inspector_view_model> m_vm;
        std::string m_title = "Inspector";
        bool m_is_open = true;
    };

} // namespace vesta