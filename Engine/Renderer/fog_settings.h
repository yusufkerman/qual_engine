#pragma once

/**
 * @file fog_settings.h
 * @brief Sahne ve oyun içi üstel sis (Exponential Fog) parametrelerini barındıran yapılandırma yapısı.
 */

#include <glm/glm.hpp>

/**
 * @struct fog_settings
 * @brief Sis efektinin aktiflik durumu, rengi, yoğunluğu ve editör sahne görünürlüğünü tutar.
 */
struct fog_settings
{
    bool enabled = true;                    ///< Sis efektinin genel olarak açık/kapalı durumu.
    bool show_in_scene = true;              ///< Sahne (Scene Viewport) penceresinde sisin gösterilip gösterilmeyeceği.
    glm::vec3 color{ 0.70f, 0.80f, 0.90f }; ///< Sis rengi (RGB).
    float density = 0.008f;                 ///< Üstel sis yoğunluk katsayısı (Exponential Fog Density).
};

