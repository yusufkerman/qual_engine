#pragma once

/**
 * @file viewport_state.h
 * @brief Tekil bir görünüm penceresinin (Viewport) anlık boyut, konum ve odak durumlarını barındıran veri yapısı.
 */

#include <glm/glm.hpp>

namespace vesta
{
    /**
     * @struct viewport_state
     * @brief ImGui panel ölçüleri ve Framebuffer çizim parametrelerini taşıyan durum yapısı.
     */
    struct viewport_state
    {
        glm::vec2 size{ 1920.0f, 1080.0f };         ///< ImGui penceresinin mevcut içerik alanı boyutu.
        glm::ivec2 render_size{ 1920.0f, 1080.0f }; ///< Gerçek Framebuffer / Doku çözünürlüğü.
        glm::vec2 position{ 0.0f, 0.0f };           ///< Panelin ekran üzerindeki sol-üst piksel koordinatı.

        unsigned int texture_id = 0;                ///< Viewport üzerinde basılan OpenGL FBO renk dokusu kimliği.

        bool focused = false;                       ///< Pencere anlık olarak klavye/girdi odağına sahip mi.
        bool hovered = false;                       ///< Fare imleci pencere sınırları içerisinde mi.
    };

} // namespace vesta