#pragma once

/**
 * @file game_viewport_panel.h
 * @brief Editör içinde oyunun son halinin (Game View) çizildiği ImGui panelini tanımlar.
 */

#include "IEditorPanel.h"
#include "../ViewModels/game_viewport_view_model.h"
#include "../CoreState/editor_viewport_state.h"
#include <string>

namespace vesta {

    /**
     * @class game_viewport_panel
     * @brief Oyun içi kameradan render edilen Framebuffer dokusunu ekrana basan ve girdileri (input) takip eden sınıf.
     */
    class game_viewport_panel : public IEditorPanel
    {
    public:
        /**
         * @brief Oyun görünümü panelini başlatır.
         *
         * @param vm Panel mantığını ve doku akışını yönetecek olan ViewModel.
         * @param viewportState Editörün genel görünüm (viewport) durum yöneticisi.
         */
        game_viewport_panel(
            game_viewport_view_model& vm,
            editor_viewport_state& viewportState);

        // ==========================================
        // IEditorPanel ARAYÜZÜ (INTERFACE)
        // ==========================================

        void render() override;
        const std::string& get_title() const override;
        bool is_open() const override;
        void set_open(bool open) override;

    private:
        game_viewport_view_model& m_vm;
        editor_viewport_state& m_viewport_state;

        std::string m_title = "Game";
        bool m_is_open = true;
    };

} // namespace vesta