#pragma once

/**
 * @file render_settings.h
 * @brief Motorun tüm grafik boru hattı (Culling, Shadows, Skybox, VSync, Rasterizer, PS1, Volumetric, SSAO/GTAO, PBR/ToneMap, GI/SSGI, AA) ayarlarını merkezi olarak yöneten yapı.
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

    // ==========================================
    // 6. HACİMSEL IŞIKLANDIRMA (VOLUMETRIC LIGHTING / GOD RAYS)
    // ==========================================
    bool enable_volumetric_lighting = true;      ///< Hacimsel ışık süzmeleri (Volumetric God Rays) aktif mi?
    float volumetric_intensity = 1.0f;           ///< Hacimsel ışık parlaklık / yoğunluk katsayısı.
    int volumetric_steps = 24;                   ///< Işın yürütme (Raymarching) örneklem adım sayısı (8 - 64).
    float volumetric_scattering = 0.6f;          ///< Henyey-Greenstein asimetri saçılma faktörü g (0.0 ile 0.95 arası; pozitif = ileriye saçılma).
    float volumetric_density = 0.04f;            ///< Sis / partikül ortam yoğunluğu (Extinction/Absorption).
    float volumetric_max_distance = 60.0f;       ///< Işın yürütme azami hesaplama mesafesi (Metre).

    // ==========================================
    // 7. ORTAM GÖLGELEMESİ (AMBIENT OCCLUSION / SSAO & GTAO)
    // ==========================================
    bool enable_ssao = true;                     ///< Ekran uzayı ortam örtmesi (AO) aktif mi?
    int ao_method = 1;                           ///< 0: SSAO (Klasik), 1: GTAO (Ground Truth Ambient Occlusion)
    float ssao_radius = 2.5f;                    ///< Örnekleme yarıçapı (Metre cinsinden view space yarıçapı).
    float ssao_bias = 0.025f;                    ///< Derinlik toleransı (Acne önleme).
    float ssao_intensity = 1.3f;                 ///< Gölgeleme koyuluk / güç katsayısı.
    int ssao_sample_count = 16;                  ///< SSAO örneklem sayısı (8 - 32).
    bool ssao_blur = true;                       ///< Kenar koruyucu bilateral bulanıklaştırma aktif mi?
    bool show_ssao_debug = false;                ///< AO dokusunu doğrudan ekranda maske olarak önizle.

    // GTAO Spesifik Ayarlar:
    int gtao_directions = 3;                     ///< GTAO dilim/yön sayısı (1 - 6).
    int gtao_steps = 4;                          ///< Her dilimdeki örnekleme adım sayısı (2 - 8).
    float gtao_falloff = 1.5f;                   ///< Mesafe sönümleme katsayısı (Haloları engeller).
    float gtao_thickness = 0.5f;                 ///< İnce nesneler için arka derinlik toleransı (Metre).

    // ==========================================
    // 8. STANDART PBR BORU HATTI, IBL VE HDR TONE MAPPING
    // ==========================================
    float pbr_ambient_intensity = 0.35f;         ///< Diffuse IBL / Hemisphere ortam aydınlanma şiddeti (0.0 - 2.0).
    float pbr_ibl_intensity = 0.15f;             ///< Specular IBL küp harita yansıma şiddeti (0.0 - 2.0, varsayılan düşürülmüş değer).
    float pbr_specular_intensity = 1.0f;         ///< Cook-Torrance GGX doğrudan parlama (specular) çarpanı.
    float pbr_roughness_multiplier = 1.0f;       ///< Küresel pürüzlülük (roughness) çarpanı.
    float pbr_metallic_multiplier = 1.0f;        ///< Küresel metaliklik (metallic) çarpanı.
    float pbr_normal_strength = 1.0f;            ///< Küresel normal haritası (normal map) derinlik çarpanı.
    float pbr_exposure = 0.0f;                   ///< HDR Pozlama (EV - Exposure Value, [-4.0 .. +4.0], çarpan = 2^EV).
    float pbr_white_point = 4.0f;                ///< Reinhard ve Uncharted 2 için beyaz nokta (White Point) eşiği.
    int pbr_tone_mapping = 2;                    ///< Tone Mapping operatörü (0: None/Clamp, 1: Reinhard, 2: ACES Filmic, 3: AgX, 4: Uncharted 2 / Filmic).

    // ==========================================
    // 9. DOLAYLI IŞIKLANDIRMA (INDIRECT DIFFUSE GI: SSGI & HEMISPHERE)
    // ==========================================
    bool enable_gi = true;                       ///< Dolaylı yayılı ışıklandırma (Indirect Diffuse GI) aktif mi?
    int gi_method = 1;                           ///< 0: Hemisphere Sky/Ground Irradiance, 1: Hybrid Screen-Space Single-Bounce SSGI + Hemisphere.
    float gi_intensity = 1.2f;                   ///< SSGI sekme ışığı (bounce light) şiddeti çarpanı.
    float gi_radius = 4.5f;                      ///< SSGI ekran/görüş uzayı sekme yarıçapı (Metre).
    int gi_sample_count = 6;                     ///< SSGI piksel başına ışın/örneklem sayısı (2 - 16).
    glm::vec3 gi_sky_color{ 0.58f, 0.70f, 0.92f };    ///< Üst yarım-küre gökyüzü ortam ışığı rengi (Ambient Sky Color).
    glm::vec3 gi_ground_color{ 0.45f, 0.36f, 0.26f }; ///< Alt yarım-küre zemin yansıma rengi (Ambient Ground Bounce Color).

    // ==========================================
    // 10. KENAR YUMUŞATMA (ANTI-ALIASING: FXAA & TAA)
    // ==========================================
    int aa_mode = 2;                             ///< 0: Off, 1: FXAA 3.11 (LDR), 2: TAA (Halton Jitter + Variance Clipping).
    float taa_blend = 0.88f;                     ///< TAA geçmiş kare geri besleme (feedback/blend) ağırlığı (0.50 - 0.96).
    float taa_sharpen = 0.25f;                   ///< TAA / FXAA sonrası keskinleştirme (Sharpen) miktarı (0.0 - 1.0).
};
