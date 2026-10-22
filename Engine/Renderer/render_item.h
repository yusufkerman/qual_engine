#pragma once

/**
 * @file render_item.h
 * @brief Sahnedeki tekil bir alt ağın (submesh) GPU çizimi için gerekli tüm verileri taşıyan yapı.
 */

#include <glm/glm.hpp>
#include "../Engine/Assets/aria.h"

 /**
  * @struct render_item
  * @brief Dönüşüm matrisi, Aria varlık tanıtıcıları (handle) ve modern PBR materyal parametrelerini barındıran çizim emri paketi.
  */
struct render_item
{
    glm::mat4 model{ 1.0f };                                    ///< Modelin dünya uzayındaki dönüşüm matrisi.

    aria::gpu_shader_id gpu_shader_id{ 0 };                     ///< Çizimde kullanılacak aktif GPU shader kimliği (Toplu çizim anahtarı).

    // Ağ (Mesh) referansları
    bool is_skinned{ false };                                   ///< Çizimin iskeletli (derili) model olup olmadığını belirler.
    aria::handle<aria::model_asset> model_handle;               ///< Aria statik model varlık tanıtıcısı.
    aria::handle<aria::skinned_model_asset> skinned_model_handle; ///< Aria derili model varlık tanıtıcısı.
    size_t sub_mesh_index{ 0 };                                 ///< Çizilecek alt ağ (submesh) indeksi.

    // İskelet / Deri Yüzme (Skinning) Matris Referansları
    unsigned int bone_ubo{ 0 };                                 ///< Varsa doğrudan bağlanacak kemik UBO kimliği.
    const std::vector<glm::mat4>* bone_matrices{ nullptr };     ///< CPU üzerindeki anlık kemik matris paleti referansı.

    // Doku tanıtıcıları
    aria::handle<aria::texture_asset> diffuse_handle;           ///< Albedo / Diffuse doku bileti.
    aria::handle<aria::texture_asset> normal_handle;            ///< Teğet uzayı normal haritası bileti.
    aria::handle<aria::texture_asset> metallic_handle;          ///< Metaliklik haritası bileti (PBR).
    aria::handle<aria::texture_asset> roughness_handle;         ///< Pürüzlülük haritası bileti (PBR).
    aria::handle<aria::texture_asset> ao_handle;                ///< Ambient Occlusion haritası bileti (PBR).
    aria::handle<aria::texture_asset> emissive_handle;          ///< Kendiliğinden ışıma haritası bileti (PBR).
    aria::handle<aria::texture_asset> specular_handle;          ///< Speküler yansıma haritası bileti (Legacy).

    // Modern PBR uniform parametreleri
    glm::vec3 base_color{ 1.0f };                               ///< Yüzey ana renk çarpanı (Albedo tint).
    float metallic{ 0.0f };                                     ///< Metaliklik çarpanı (0.0: Dielektrik, 1.0: Saf metal).
    float roughness{ 0.5f };                                    ///< Yüzey pürüzlülüğü (0.0: Parlak ayna, 1.0: Mat dağınık).
    float ao{ 1.0f };                                           ///< Ortam örtme (AO) katsayısı.
    glm::vec3 emissive_color{ 0.0f };                           ///< Işıma rengi.
    float emissive_intensity{ 0.0f };                           ///< Işıma yoğunluğu.

    glm::vec2 texture_scale{ 1.0f };                            ///< UV doku ölçekleme katsayısı.
    float normal_strength{ 1.0f };                              ///< Normal haritası etki şiddeti.

    // Geriye dönük uyumluluk parametreleri
    float specular_strength{ 0.5f };                            ///< Speküler yansıma yoğunluğu (Legacy).
    float shininess{ 32.0f };                                   ///< Speküler parlaklık üssü (Legacy).

    // Şeffaflık ve alfa kırpma parametreleri
    aria::render_mode mode{ aria::render_mode::opaque };         ///< Çizim modu (Opaque, Transparent, Cutout).
    float alpha_threshold{ 0.1f };                              ///< Alfa test eşik değeri.
};