#pragma once

/**
 * @file render_settings.h
 * @brief Motorun tüm grafik boru hattı (Culling, Shadows, Skybox, VSync, Rasterizer, PS1) ayarlarını merkezi olarak yöneten yapı.
 */

#include <glm/glm.hpp>
#include <string>

/**
 * @struct render_settings
 * @brief Arka uç render parametrelerini ve grafik yapılandırmasını tutar.
 */
struct render_settings
{
    // ==========================================
    // 1. GÖRÜŞ ALANI KIRPMA (FRUSTUM CULLING)
    // ==========================================
    bool enable_frustum_culling = true;         ///< Frustum culling aktif mi?
    float cull_bias = 0.01f;                    ///< Kırpma sınır tolerans payı (kCullBias).

    // ==========================================
    // 2. GÖLGE HARİTALAMA (SHADOW MAPPING)
    // ==========================================
    bool enable_shadows = true;                 ///< Gölge haritalama aktif mi?
    int shadow_map_resolution = 2048;           ///< Gölge tamponu çözünürlüğü (512, 1024, 2048, 4096).
    float shadow_ortho_size = 150.0f;           ///< Yönlü ışık gölge projeksiyon hacmi boyutu (Ortho half-extent).
    float shadow_near_plane = 0.1f;             ///< Gölge kamerası yakın düzlem mesafesi.
    float shadow_far_plane = 500.0f;            ///< Gölge kamerası uzak düzlem mesafesi.
    float shadow_distance = 150.0f;             ///< Kameradan ışık kaynağına olan gölge takip mesafesi.
    float shadow_camera_height = 100.0f;        ///< Gölge projeksiyon merkezinin düşey yüksekliği.
    bool show_shadow_map_debug = false;         ///< Arayüzde gölge derinlik haritasını önizleme olarak göster.
    bool enable_csm = true;                     ///< Kaskatlı gölge haritalama (CSM) aktif mi?
    int csm_cascade_count = 3;                  ///< Kaskat sayısı (3).
    float csm_split_distances[3] = { 15.0f, 50.0f, 150.0f }; ///< Kaskat ayrım mesafeleri (Metre).

    // ==========================================
    // 3. GÖKYÜZÜ KUTUSU (SKYBOX)
    // ==========================================
    bool enable_skybox = true;                  ///< Skybox çizimi aktif mi?
    std::string skybox_path = "@engine/Textures/sky/skybox1.png"; ///< Yüklü olan skybox atlas dosya yolu.
    glm::vec3 clear_color_scene{ 0.08f, 0.08f, 0.12f }; ///< Skybox kapalıyken Sahne arka plan temizleme rengi.
    glm::vec3 clear_color_game{ 0.03f, 0.03f, 0.05f };  ///< Skybox kapalıyken Oyun arka plan temizleme rengi.

    // ==========================================
    // 4. RASTERİZER VE DONANIM (RASTERIZER & HARDWARE)
    // ==========================================
    bool vsync = true;                          ///< Dikey senkronizasyon (VSync).
    bool wireframe = false;                     ///< Tel kafes (Wireframe) çizim modu.
    bool backface_culling = false;              ///< Arka yüz kırpma (GL_CULL_FACE).

    // ==========================================
    // 5. PS1 RETRO HORROR PIPELINE
    // ==========================================
    bool enable_ps1_mode = false;               ///< PS1 retro estetik boru hattı aktif mi?
    bool ps1_downscale = true;                  ///< PS1 düşük çözünürlüklü pikselleştirme (Downscale / Chunky Pixel) aktif mi?
    int ps1_resolution_preset = 0;              ///< Çözünürlük hazır seti (0: 320x240 PS1 4:3, 1: 426x240 PS1 16:9, 2: 640x480 Hi-Res, 3: 256x224 Low, 4: Dinamik 0.25x, 5: Ozel)
    int ps1_custom_width = 320;                 ///< Özel render genişliği
    int ps1_custom_height = 240;                ///< Özel render yüksekliği
    float ps1_vertex_snapping = 240.0f;         ///< Tepe noktası ızgara kenetleme çözünürlüğü (Wobble/Jitter: 120, 160, 240, 320, 0 = kapalı).
    bool ps1_affine_texture_mapping = true;     ///< Doğrusal afin doku haritalama (Perspektif düzeltmesiz doku yüzme etkisi).
    float ps1_color_depth = 32.0f;              ///< Renk derinliği kuantizasyonu (32 = 15-bit RGB555 PS1 standartı, 16 = 12-bit).
    bool ps1_dithering = true;                  ///< 4x4 Bayer matrisi sıralı dithering (Ordered Dithering).
    float ps1_dither_strength = 1.0f;           ///< Dithering matrisi etki gücü çarpanı.
};
