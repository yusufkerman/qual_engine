#pragma once

/**
 * @file gizmo_settings.h
 * @brief Sahne görünümü (Scene Viewport) manipülasyon araçları, ızgara, billboard ikonları ve bileşen gizmolarının merkezi yapılandırma yapısı.
 */

#include <cstdint>
#include <unordered_map>

/**
 * @enum gizmo_tool_mode
 * @brief Sahne görünümünde aktif olan temel dönüşüm/seçim aracını belirtir.
 */
enum class gizmo_tool_mode : int
{
    select = 0,     ///< Sadece seçim (Q) - Dönüşüm tutamaçları gizlenir.
    translate = 1,  ///< Konum taşıma aracı (W).
    rotate = 2,     ///< Döndürme aracı (E).
    scale = 3       ///< Ölçeklendirme aracı (R).
};

/**
 * @enum gizmo_space
 * @brief Dönüşüm aracının eksen hizalama uzayını belirtir.
 */
enum class gizmo_space : int
{
    world = 0,      ///< Dünya eksenlerine (Global X, Y, Z) hizalı.
    local = 1       ///< Seçili nesnenin kendi yerel yönelimine (Local) hizalı.
};

/**
 * @enum gizmo_visibility_mode
 * @brief Bileşen gizmolarının (Collider, Kamera Frustum, Işık Sınırları) hangi durumda çizileceğini belirler.
 */
enum class gizmo_visibility_mode : int
{
    off = 0,            ///< Hiçbir zaman çizilmez.
    selected_only = 1,  ///< Sadece seçili varlık için çizilir.
    always = 2          ///< Sahnedeki ilgili tüm varlıklar için her zaman çizilir.
};

/**
 * @enum entity_icon_type
 * @brief Görünmez nesneler veya özel işaretleyiciler için Unity tarzı Sahne/Hiyerarşi ikon tipleri.
 */
enum class entity_icon_type : int
{
    auto_detect = 0,        ///< Bileşene göre otomatik belirle (Kamera, Işık vb.).
    none = 1,               ///< İkon gösterme.
    camera = 2,             ///< Kamera ikonu.
    directional_light = 3,  ///< Yönlü ışık (Güneş) ikonu.
    point_light = 4,        ///< Noktasal ışık (Ampul) ikonu.
    spot_light = 5,         ///< Spot ışık (Konik projektör) ikonu.
    spawn_point = 6,        ///< Doğma noktası (Bayrak) ikonu.
    trigger_zone = 7,       ///< Tetikleyici bölge (Elmas) ikonu.
    audio_source = 8,       ///< Ses kaynağı (Hoparlör) ikonu.
    waypoint = 9,           ///< Rota/Hedef noktası (Nişangah) ikonu.
    label_blue = 10,        ///< Mavi etiket rozeti.
    label_green = 11,       ///< Yeşil etiket rozeti.
    label_yellow = 12,      ///< Sarı etiket rozeti.
    label_orange = 13,      ///< Turuncu etiket rozeti.
    label_red = 14,         ///< Kırmızı etiket rozeti.
    label_purple = 15       ///< Mor etiket rozeti.
};

/**
 * @struct gizmo_settings
 * @brief Editör ve çalışma zamanı gizmo modülünün tüm görsel ve etkileşimsel parametrelerini tutar.
 */
struct gizmo_settings
{
    // ========================================================================
    // 1. DÖNÜŞÜM ARAÇLARI (TRANSFORM TOOLS)
    // ========================================================================
    gizmo_tool_mode active_tool = gizmo_tool_mode::translate;
    gizmo_space transform_space = gizmo_space::local;

    bool snap_enabled = false;          ///< Adımlı kenetlenme (Snapping) aktif mi?
    float translate_snap = 0.5f;        ///< Konum kenetlenme adımı (birim/metre).
    float rotate_snap_deg = 15.0f;      ///< Açı kenetlenme adımı (derece).
    float scale_snap = 0.25f;           ///< Ölçek kenetlenme adımı.

    float handle_size = 95.0f;          ///< Dönüşüm gizmosunun ekran uzayındaki boyutu (piksel).

    // ========================================================================
    // 2. GENEL GİZMO VE ÇİZGİ AYARLARI
    // ========================================================================
    bool enabled = true;                ///< Tüm 3B gizmo ve ikon çizimlerini etkinleştirir/kapatır.
    float line_width = 2.0f;            ///< 3B tel kafes (wireframe) gizmo çizgi kalınlığı.

    // ========================================================================
    // 3. SONSUZ IZGARA (GRID) AYARLARI
    // ========================================================================
    bool show_grid = true;              ///< Sahne referans ızgarasını göster.
    bool show_grid_axes = true;         ///< X (Kırmızı) ve Z (Mavi) ana eksen çizgilerini vurgula.
    float grid_opacity = 0.85f;         ///< Izgara genel saydamlık çarpanı [0.1 - 1.0].

    // ========================================================================
    // 4. BİLLBOARD İKON VE HİYERARŞİ ROZET AYARLARI
    // ========================================================================
    bool show_icons = true;             ///< Sahne içindeki kamera, ışık ve özel nesne ikonlarını göster.
    float icon_size = 1.0f;             ///< Billboard ikon boyut ölçeği [0.3 - 2.5].
    bool icon_distance_scaling = true;  ///< İkonların kamera mesafesine göre perspektif küçülmesi.
    bool show_hierarchy_badges = true;  ///< Hiyerarşi panelinde nesne ikon rozetlerini göster.

    // ========================================================================
    // 5. BİLEŞEN GİZMOLARI (COLLIDER, CAMERA, LIGHT)
    // ========================================================================
    gizmo_visibility_mode collider_mode = gizmo_visibility_mode::selected_only;
    gizmo_visibility_mode camera_frustum_mode = gizmo_visibility_mode::selected_only;
    bool clamp_camera_frustum_dist = true;          ///< Çok uzak farPlane değerlerini sahnede okunaklı tutmak için sınırla.
    float camera_frustum_max_draw_dist = 25.0f;     ///< Görsel kamera frustum önizleme mesafe sınırı.

    gizmo_visibility_mode light_gizmo_mode = gizmo_visibility_mode::selected_only;
    bool interactive_light_handles = true;          ///< Sahne üzerinden ışık menzili (range) ve koni açısı (spot angle) tutamaçlarını sürükleyerek düzenleme.

    // ========================================================================
    // 6. GAMEPLAY / ÇALIŞMA ZAMANI HATA AYIKLAMA GİZMOLARI VE PERFORMANS PANELİ
    // ========================================================================
    bool show_gameplay_gizmos_in_scene = true;      ///< Gameplay API çizgilerini Scene penceresinde göster.
    bool show_gameplay_gizmos_in_game = false;      ///< Gameplay API çizgilerini Game penceresinde göster.
    bool show_stats_overlay = false;                ///< Sahne görünümünde (Scene Viewport) gerçek zamanlı FPS ve render istatistikleri penceresini göster.

    // ========================================================================
    // 7. VARLIK BAŞINA ÖZEL İKON ATAMALARI (UNITY TARZI İKON SEÇİMİ)
    // ========================================================================
    std::unordered_map<uint32_t, entity_icon_type> custom_entity_icons;

    /**
     * @brief Bir varlığa atanmış özel ikon tipini döndürür (Atanmamışsa auto_detect döner).
     */
    [[nodiscard]] entity_icon_type get_entity_icon(uint32_t e) const
    {
        auto it = custom_entity_icons.find(e);
        if (it != custom_entity_icons.end())
        {
            return it->second;
        }
        return entity_icon_type::auto_detect;
    }

    /**
     * @brief Bir varlığa özel sahne/hiyerarşi ikonu atar.
     */
    void set_entity_icon(uint32_t e, entity_icon_type icon)
    {
        if (icon == entity_icon_type::auto_detect)
        {
            custom_entity_icons.erase(e);
        }
        else
        {
            custom_entity_icons[e] = icon;
        }
    }

    /**
     * @brief Tüm gizmo ayarlarını varsayılan fabrika değerlerine sıfırlar.
     */
    void reset_defaults()
    {
        active_tool = gizmo_tool_mode::translate;
        transform_space = gizmo_space::local;
        snap_enabled = false;
        translate_snap = 0.5f;
        rotate_snap_deg = 15.0f;
        scale_snap = 0.25f;
        handle_size = 95.0f;

        enabled = true;
        line_width = 2.0f;

        show_grid = true;
        show_grid_axes = true;
        grid_opacity = 0.85f;

        show_icons = true;
        icon_size = 1.0f;
        icon_distance_scaling = true;
        show_hierarchy_badges = true;

        collider_mode = gizmo_visibility_mode::selected_only;
        camera_frustum_mode = gizmo_visibility_mode::selected_only;
        clamp_camera_frustum_dist = true;
        camera_frustum_max_draw_dist = 25.0f;

        light_gizmo_mode = gizmo_visibility_mode::selected_only;
        interactive_light_handles = true;

        show_gameplay_gizmos_in_scene = true;
        show_gameplay_gizmos_in_game = false;
        show_stats_overlay = false;
    }
};

