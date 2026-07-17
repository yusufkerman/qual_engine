#pragma once

#include "viewport_state.h"

namespace vesta
{
    class editor_viewport_state
    {
    public:

        //========================
        // Scene Viewport
        //========================

        viewport_state& scene()
        {
            return m_scene;
        }

        const viewport_state& scene() const
        {
            return m_scene;
        }

        //========================
        // Game Viewport
        //========================

        viewport_state& game()
        {
            return m_game;
        }

        const viewport_state& game() const
        {
            return m_game;
        }

    private:

        viewport_state m_scene;
        viewport_state m_game;
    };
}