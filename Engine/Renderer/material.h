#pragma once

/**
 * @file material.h
 * @brief Geometrilerin yüzey görünümünü, dokularını ve aydınlatma tepkilerini tanımlayan materyal yapısı.
 */

#include <glm/glm.hpp>

 /**
  * @struct material
  * @brief Shader programı, doku kimlikleri ve aydınlatma parametrelerini barındıran veri yapısı.
  */
struct material
{
    unsigned int shader_id = 0;              ///< Kullanılacak OpenGL shader programı tanıtıcısı.

    glm::vec3 base_color{ 1.0f, 1.0f, 1.0f }; ///< Yüzey ana renk çarpanı (Albedo/Diffuse color tint).

    unsigned int diffuse_texture = 0;         ///< Diffuse/Albedo renk dokusu kimliği (Texture ID).
    unsigned int normal_texture = 0;          ///< Teğet uzayı normal haritası doku kimliği.
    unsigned int specular_texture = 0;        ///< Parlaklık (Specular/Gloss) haritası doku kimliği.

    float normal_strength = 1.0f;             ///< Normal haritasının yüzey eğimine etki şiddeti.

    glm::vec2 texture_scale{ 1.0f, 1.0f };    ///< UV koordinat ölçekleme çarpanı (Tiling).

    float specular_strength = 0.5f;           ///< Speküler yansıma yoğunluğu.
    float shininess = 32.0f;                  ///< Speküler parlaklık üssü (Specular exponent).
};