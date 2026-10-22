#pragma once

/**
 * @file material.h
 * @brief Geometrilerin yüzey görünümünü, PBR (Physically Based Rendering) özelliklerini,
 * dokularını ve aydınlatma tepkilerini tanımlayan modern materyal yapısı.
 */

#include <glm/glm.hpp>

/**
 * @struct material
 * @brief PBR Cook-Torrance BRDF (Metallic-Roughness) iş akışını ve doku parametrelerini barındıran veri yapısı.
 */
struct material
{
    unsigned int shader_id = 0;                   ///< Kullanılacak OpenGL shader programı tanıtıcısı.

    // ==========================================
    // MODERN PBR (METALLIC / ROUGHNESS) PARAMETRELERİ
    // ==========================================
    glm::vec3 base_color{ 1.0f, 1.0f, 1.0f };     ///< Yüzey ana renk çarpanı (Albedo / Base Color tint).
    float metallic = 0.0f;                        ///< Metaliklik katsayısı (0.0: Dielektrik/Yalıtkan, 1.0: Saf Metal).
    float roughness = 0.5f;                       ///< Yüzey mikroskobik pürüzlülüğü (0.0: Parlak ayna, 1.0: Tam mat dağınık).
    float ao = 1.0f;                              ///< Ortam örtme (Ambient Occlusion) çarpanı [0.0..1.0].

    // ==========================================
    // IŞIMA (EMISSIVE) AYARLARI
    // ==========================================
    glm::vec3 emissive_color{ 0.0f, 0.0f, 0.0f }; ///< Kendiliğinden ışık yayan yüzeyler için ışıma rengi.
    float emissive_intensity = 0.0f;               ///< Işıma yoğunluğu/şiddeti çarpanı.

    // ==========================================
    // DOKU TANITICILARI (OPENGL TEXTURE HANDLES)
    // ==========================================
    unsigned int diffuse_texture = 0;             ///< Albedo / Diffuse renk dokusu kimliği.
    unsigned int normal_texture = 0;              ///< Teğet uzayı normal haritası doku kimliği.
    unsigned int metallic_texture = 0;            ///< Metaliklik haritası doku kimliği (PBR).
    unsigned int roughness_texture = 0;           ///< Pürüzlülük haritası doku kimliği (PBR).
    unsigned int ao_texture = 0;                  ///< Ortam örtme (AO) dokusu kimliği (PBR).
    unsigned int emissive_texture = 0;            ///< Işıma haritası doku kimliği (PBR).
    unsigned int specular_texture = 0;            ///< Eski nesil yansıma haritası doku kimliği (Geriye dönük uyumluluk).

    float normal_strength = 1.0f;                 ///< Normal haritasının yüzey eğimine etki şiddeti.
    glm::vec2 texture_scale{ 1.0f, 1.0f };        ///< UV koordinat ölçekleme çarpanı (Tiling).

    // ==========================================
    // GERİYE DÖNÜK UYUMLULUK (LEGACY FIELDS)
    // ==========================================
    float specular_strength = 0.5f;               ///< Eski nesil speküler yansıma yoğunluğu.
    float shininess = 32.0f;                      ///< Eski nesil speküler üssü.
};