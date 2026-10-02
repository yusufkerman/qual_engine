#pragma once

/**
 * @file render_item.h
 * @brief Sahnedeki tekil bir alt ağın (submesh) GPU çizimi için gerekli tüm verileri taşıyan yapı.
 */

#include <glm/glm.hpp>
#include "../Engine/Assets/aria.h"

 /**
  * @struct render_item
  * @brief Dönüşüm matrisi, Aria varlık tanıtıcıları (handle) ve materyal parametrelerini barındıran çizim emri paketi.
  */
struct render_item
{
    glm::mat4 model{ 1.0f };                                    ///< Modelin dünya uzayındaki dönüşüm matrisi.

    aria::gpu_shader_id gpu_shader_id{ 0 };                     ///< Çizimde kullanılacak aktif GPU shader kimliği (Toplu çizim anahtarı).

    // Ağ (Mesh) referansları
    aria::handle<aria::model_asset> model_handle;               ///< Aria model varlık tanıtıcısı.
    size_t sub_mesh_index{ 0 };                                 ///< Çizilecek alt ağ (submesh) indeksi.

    // Doku tanıtıcıları
    aria::handle<aria::texture_asset> diffuse_handle;           ///< Albedo/Diffuse doku bileti.
    aria::handle<aria::texture_asset> normal_handle;            ///< Teğet uzayı normal haritası bileti.
    aria::handle<aria::texture_asset> specular_handle;          ///< Speküler yansıma haritası bileti.

    // Materyal uniform parametreleri
    glm::vec3 base_color{ 1.0f };                               ///< Yüzey ana renk çarpanı.
    glm::vec2 texture_scale{ 1.0f };                            ///< UV doku ölçekleme katsayısı.
    float normal_strength{ 1.0f };                              ///< Normal haritası etki şiddeti.
    float specular_strength{ 0.5f };                            ///< Speküler yansıma yoğunluğu.
    float shininess{ 32.0f };                                   ///< Speküler parlaklık üssü.

    // Şeffaflık ve alfa kırpma parametreleri
    aria::render_mode mode{ aria::render_mode::opaque };         ///< Çizim modu (Opaque, Transparent, Cutout).
    float alpha_threshold{ 0.1f };                              ///< Alfa test eşik değeri.
};