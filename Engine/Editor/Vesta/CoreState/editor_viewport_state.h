#pragma once

/**
 * @file editor_viewport_state.h
 * @brief Editörün Sahne (Scene) ve Oyun (Game) pencerelerinin anlık durum verilerini yöneten taşıyıcı sınıf.
 */

#include "viewport_state.h"

namespace vesta
{
    /**
     * @class editor_viewport_state
     * @brief Sahne ve Oyun viewport'larının odak, boyut ve konum durumlarını tek bir merkezde saklar.
     */
    class editor_viewport_state
    {
    public:
        // ==========================================
        // SAHNE PENCERESİ (SCENE VIEWPORT)
        // ==========================================

        /**
         * @brief Sahne viewport durumuna değiştirilebilir (mutable) referans döndürür.
         */
        viewport_state& scene()
        {
            return m_scene;
        }

        /**
         * @brief Sahne viewport durumuna salt okunur (const) referans döndürür.
         */
        const viewport_state& scene() const
        {
            return m_scene;
        }

        // ==========================================
        // OYUN PENCERESİ (GAME VIEWPORT)
        // ==========================================

        /**
         * @brief Oyun viewport durumuna değiştirilebilir (mutable) referans döndürür.
         */
        viewport_state& game()
        {
            return m_game;
        }

        /**
         * @brief Oyun viewport durumuna salt okunur (const) referans döndürür.
         */
        const viewport_state& game() const
        {
            return m_game;
        }

    private:
        viewport_state m_scene; ///< Sahne düzenleme ekranının durumu
        viewport_state m_game;  ///< Oyun kamerasından render edilen ekranın durumu
    };

} // namespace vesta