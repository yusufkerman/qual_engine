#pragma once

/**
 * @file lighting_panel.h
 * @brief Sahne ışıklandırma ve üstel sis (Exponential Fog) parametrelerini yöneten ImGui editör paneli.
 */

#include "IEditorPanel.h"
#include <string>

struct EngineContext;

namespace vesta {

    /**
     * @class lighting_panel
     * @brief Sis açma/kapama, sis rengi, sis yoğunluğu ve genel ışıklandırma ayarlarını düzenleyen panel.
     */
    class lighting_panel : public IEditorPanel
    {
    public:
        explicit lighting_panel(EngineContext& engineCtx);
        ~lighting_panel() override = default;

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }

    private:
        EngineContext& m_engine_ctx;
        std::string m_title = "Lighting";
        bool m_is_open = false; // Varsayılan olarak kapalı, Window -> Rendering/Lighting ile açılır
    };

} // namespace vesta

