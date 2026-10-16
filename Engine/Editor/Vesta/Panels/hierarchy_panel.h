#pragma once

/**
 * @file hierarchy_panel.h
 * @brief Sahne Hiyerarşisi (Hierarchy) paneli için kullanıcı arayüzü (View) katmanını tanımlar.
 */

#include "IEditorPanel.h"
#include "../ViewModels/hierarchy_view_model.h"
#include "../../../Renderer/Debug/gizmo_settings.h"
#include <memory>
#include <string>

namespace vesta {

    /**
     * @class hierarchy_panel
     * @brief Sahnedeki varlıkları (entities) ağaç yapısında çizen, sürükle-bırak, ikon rozeti ve seçim işlemlerini yöneten ImGui paneli.
     * @note Kullanıcı etkileşimlerini doğrudan hierarchy_view_model (MVVM) üzerine delege eder.
     */
    class hierarchy_panel : public IEditorPanel {
    public:
        /**
         * @brief Hiyerarşi panelini başlatır ve görünüm modelini (ViewModel) oluşturur.
         *
         * @param world Sahne işlemlerini yöneten dünya arayüzü.
         * @param reg Varlıkların barındığı ECS kayıt defteri.
         * @param selection Editör genelindeki seçim yöneticisi.
         * @param gizmoSettings İkon rozetleri ve özel ikon atamaları için gizmo ayarları işaretçisi.
         */
        hierarchy_panel(
            IEditableWorld& world,
            registry& reg,
            editor_selection_manager& selection,
            gizmo_settings* gizmoSettings = nullptr);
        ~hierarchy_panel() override = default;

        // ==========================================
        // IEditorPanel ARAYÜZÜ (INTERFACE)
        // ==========================================

        void render() override;
        const std::string& get_title() const override;
        bool is_open() const override;
        void set_open(bool open) override;

    private:
        std::unique_ptr<hierarchy_view_model> m_vm;
        gizmo_settings* m_gizmo_settings = nullptr;

        std::string m_title = "Hierarchy";
        bool m_is_open = true;
    };

} // namespace vesta