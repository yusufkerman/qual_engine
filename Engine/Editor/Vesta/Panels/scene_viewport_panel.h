#pragma once

/**
 * @file scene_viewport_panel.h
 * @brief Editör sahne düzenleme ekranının (Scene Viewport) çizildiği ImGui panelini tanımlar.
 */

#include "IEditorPanel.h"
#include "../ViewModels/scene_viewport_view_model.h"
#include "../CoreState/editor_viewport_state.h"
#include <string>

namespace vesta {

    /**
     * @class scene_viewport_panel
     * @brief Editör kamerasından render edilen Framebuffer dokusunu çizen, gizmo ve fare girdilerini takip eden sınıf.
     */
    class scene_viewport_panel : public IEditorPanel
    {
    public:
        /**
         * @brief Sahne görünümü panelini başlatır.
         *
         * @param vm Panel mantığını ve doku kimliğini yöneten ViewModel.
         * @param viewportState Editörün genel görünüm (viewport) durum yöneticisi.
         */
        scene_viewport_panel(
            scene_viewport_view_model& vm,
            editor_viewport_state& viewportState);

        ~scene_viewport_panel() override = default;

        // ==========================================
        // IEditorPanel ARAYÜZÜ (INTERFACE)
        // ==========================================

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }

    private:
        scene_viewport_view_model& m_vm;
        editor_viewport_state& m_viewport_state;

        std::string m_title = "Scene";
        bool m_is_open = true;
    };

} // namespace vesta