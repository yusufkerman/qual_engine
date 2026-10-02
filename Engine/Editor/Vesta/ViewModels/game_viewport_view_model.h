#pragma once

/**
 * @file game_viewport_view_model.h
 * @brief Oyun görünümü (Game Viewport) paneli için Model-View-ViewModel (MVVM) yapısının görünüm modeli katmanını tanımlar.
 */

#include <glm/glm.hpp>
#include "../CoreState/editor_viewport_state.h"

namespace vesta {

    /**
     * @class game_viewport_view_model
     * @brief Oyun ekranının render edildiği panelin boyut, odak (focus) ve doku (texture) durumlarını yönetir.
     */
    class game_viewport_view_model {
    public:
        /**
         * @brief Görünüm modelini global editör durumu ile başlatır.
         *
         * @param state Editörün genel görünüm durumu (viewport state) referansı.
         */
        explicit game_viewport_view_model(editor_viewport_state& state)
            : m_state(state)
        {
        }

        // ========================================================================
        // GETTERS & SETTERS
        // ========================================================================

        /**
         * @brief Oyun ekranının çizildiği hedefin (Render Target) OpenGL doku kimliğini döndürür.
         */
        unsigned int get_texture_id() const { return m_texture_id; }
        void set_texture_id(unsigned int id) { m_texture_id = id; }

        /**
         * @brief Panel içindeki render alanının boyutlarını (piksel cinsinden) döndürür.
         */
        const glm::vec2& get_size() const { return m_size; }
        void set_size(const glm::vec2& size) { m_size = size; }

        /**
         * @brief Panelin klavye/fare odağında (focus) olup olmadığını döndürür/ayarlar.
         * @note Odak (focus) durumu, karakter kontrolü ve kısayolların çalışması için kritiktir.
         */
        void set_focused(bool focused) { m_is_focused = focused; }
        bool is_focused() const { return m_is_focused; }

        /**
         * @brief Fare imlecinin panel üzerinde (hover) olup olmadığını döndürür/ayarlar.
         */
        void set_hovered(bool hovered) { m_is_hovered = hovered; }
        bool is_hovered() const { return m_is_hovered; }

        /**
         * @brief Panelin ekrandaki (editör penceresi içindeki) 2D konumunu döndürür/ayarlar.
         */
        void set_position(const glm::vec2& pos) { m_position = pos; }
        const glm::vec2& get_position() const { return m_position; }

    private:
        editor_viewport_state& m_state;     ///< Global editör görünüm durumu referansı.

        unsigned int m_texture_id{ 0 };     ///< Ekrana basılacak olan Framebuffer (FBO) dokusu.
        glm::vec2 m_size{ 800.0f, 600.0f }; ///< Panelin mevcut genişlik ve yüksekliği.
        glm::vec2 m_position{ 0.0f, 0.0f }; ///< Panelin arayüzdeki X, Y konumu.

        bool m_is_focused{ false };         ///< Panel aktif olarak kullanıcı girişlerini (input) alıyor mu?
        bool m_is_hovered{ false };         ///< Fare imleci panelin sınırları içerisinde mi?
    };

} // namespace vesta