#pragma once

/**
 * @file fog_settings.h
 * @brief Sahne ve oyun içi üstel mesafe + yükseklik sisi (Exponential Distance & Height Fog) ve güneş saçılımı parametrelerini barındıran yapılandırma yapısı.
 */

#include <glm/glm.hpp>

/**
 * @struct fog_settings
 * @brief Sis efektinin aktiflik durumu, türü (mesafe/yükseklik), rengi, yoğunluğu, güneş saçılımı ve editör sahne görünürlüğünü tutar.
 */
struct fog_settings
{
    bool enabled = true;                        ///< Sis efektinin genel olarak açık/kapalı durumu.
    bool show_in_scene = true;                  ///< Sahne (Scene Viewport) penceresinde sisin gösterilip gösterilmeyeceği.
    int mode = 1;                               ///< Sis türü (0: Exponential Distance Fog, 1: Exponential Height + Distance Fog).
    glm::vec3 color{ 0.65f, 0.75f, 0.88f };     ///< Temel sis rengi (RGB).
    float density = 0.006f;                     ///< Üstel sis yoğunluk katsayısı (Exponential Fog Density).
    float height_falloff = 0.08f;               ///< Yükseklikle sisin azalma katsayısı (Height Falloff).
    float base_height = 0.0f;                   ///< Yükseklik sisinin başladığı referans zemin yüksekliği (Metre).
    float sun_scattering_intensity = 0.65f;     ///< Güneş yönüne bakarken sisin güneş rengine kayma (Mie In-Scattering) şiddeti.
    bool link_to_ambient_sky = true;            ///< Sis rengini ortam gökyüzü/ufuk rengiyle uyumlu harmanla.
};
