#pragma once

/**
 * @file rendering_panel.h
 * @brief Tüm grafik arka uç ayarlarını (Culling, Gölgeler, Skybox, VSync, Sis) yöneten ImGui editör paneli.
 */

#include "IEditorPanel.h"
#include <string>

struct EngineContext;

namespace vesta {

    /**
     * @class rendering_panel
     * @brief Motorun sunduğu tüm render boru hattı parametrelerini düzenlenebilir arayüz kontrolleriyle sunan panel.
     */
    class rendering_panel : public IEditorPanel
    {
    public:
        explicit rendering_panel(EngineContext& engineCtx);
        ~rendering_panel() override = default;

        void render() override;
        const std::string& get_title() const override { return m_title; }
        bool is_open() const override { return m_is_open; }
        void set_open(bool open) override { m_is_open = open; }

    private:
        EngineContext& m_engine_ctx;
        std::string m_title = "Rendering Settings";
        bool m_is_open = false;
    };

} // namespace vesta

