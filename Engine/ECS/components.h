#pragma once

/**
 * @file components.h
 * @brief Motor içerisindeki tüm standart ECS bileşen yapılarını (struct) ve SERA yansıma kayıtlarını barındırır.
 */

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <functional>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include "../Engine/Assets/aria.h"
#include "../Sera/sera_component_codec.h"
#include "../Audio/audio_components.h"

class scriptable_entity;

// ============================================================================
// ENUMERASYONLAR (ENUMS)
// ============================================================================

enum class light_type
{
    directional,
    point,
    spot
};

enum class motion_type
{
    static_body,    ///< Harita elementleri, sabit nesneler.
    kinematic_body, ///< Fizikten etkilenmeyen, kodla/animasyonla hareket eden nesneler.
    dynamic_body    ///< Fizik motoru tarafından tamamen simüle edilen nesneler.
};

enum class collider_type
{
    box,
    sphere
};

enum class mesh_source_type
{
    model,
    primitive_cube,
    primitive_sphere
};

enum class canvas_render_mode
{
    screen_space_overlay,
    world_space
};

enum class canvas_scaler_mode
{
    constant_pixel_size,
    scale_with_screen_size
};

enum class ui_text_alignment
{
    top_left,
    top_center,
    top_right,
    middle_left,
    center,
    middle_right,
    bottom_left,
    bottom_center,
    bottom_right
};

enum class ui_button_state
{
    normal,
    hovered,
    pressed,
    disabled
};

// ============================================================================
// BİLEŞENLER (COMPONENTS)
// ============================================================================

/**
 * @struct canvas_component
 * @brief Tüm 2B UI hiyerarşisinin kök bileşenidir. Ekran uzayı veya dünya uzayı modlarını yönetir.
 */
struct canvas_component
{
    canvas_render_mode render_mode = canvas_render_mode::screen_space_overlay;
    canvas_scaler_mode scaler_mode = canvas_scaler_mode::scale_with_screen_size;
    glm::vec2 reference_resolution{ 1920.0f, 1080.0f };
    float match_width_or_height = 0.5f; ///< 0.0f = Genişliğe göre, 1.0f = Yüksekliğe göre ölçekleme
    int sorting_order = 0;              ///< Çizim sırası önceliği
};

/**
 * @struct rect_transform_component
 * @brief 2B UI elemanlarının anchor, pivot ve boyut koordinatlarını tanımlayan arayüz dönüşüm bileşeni.
 */
struct rect_transform_component
{
    glm::vec2 anchored_position{ 0.0f, 0.0f };
    glm::vec2 size{ 100.0f, 100.0f };
    glm::vec2 anchor_min{ 0.5f, 0.5f }; ///< [0..1] Sol-alt: (0,0), Sağ-üst: (1,1)
    glm::vec2 anchor_max{ 0.5f, 0.5f }; ///< [0..1]
    glm::vec2 pivot{ 0.5f, 0.5f };      ///< [0..1] Kendi eksen merkezi (0.5, 0.5 = tam orta)
    float rotation = 0.0f;              ///< 2B Z ekseni rotasyonu (derece)
    glm::vec2 scale{ 1.0f, 1.0f };
    bool visible = true;                ///< UI elemanının görünürlüğü

    /// @brief Sistem tarafından hesaplanan son mutlak ekran koordinatları: (x = sol, y = üst, z = genişlik, w = yükseklik)
    glm::vec4 computed_screen_rect{ 0.0f };
};

/**
 * @struct ui_text_component
 * @brief Ekran üzerinde metin çizilmesini sağlayan arayüz bileşeni.
 */
struct ui_text_component
{
    std::string text = "New Text";
    std::string font_path = "EngineResources/Fonts/Roboto-Regular.ttf";
    float font_size = 24.0f;
    glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    ui_text_alignment alignment = ui_text_alignment::center;
    bool wrap_text = false;
    float line_spacing = 1.2f;
};

/**
 * @struct ui_image_component
 * @brief Renkli kutular veya dokulu görseller çizen arayüz bileşeni.
 */
struct ui_image_component
{
    glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    aria::handle<aria::texture_asset> texture_handle;
    bool raycast_target = true; ///< Tıklama/hover etkileşimi algılansın mı?
};

/**
 * @struct ui_button_component
 * @brief Etkileşimli buton ve tıklama/durum mantığını barındıran arayüz bileşeni.
 */
struct ui_button_component
{
    bool interactable = true;
    glm::vec4 normal_color{ 0.8f, 0.8f, 0.8f, 1.0f };
    glm::vec4 hover_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 pressed_color{ 0.6f, 0.6f, 0.6f, 1.0f };
    glm::vec4 disabled_color{ 0.4f, 0.4f, 0.4f, 0.5f };

    ui_button_state current_state = ui_button_state::normal;
    std::function<void()> on_click = nullptr; ///< Tıklama olayı tetiklendiğinde çalıştırılacak fonksiyon
};

struct transform_component
{
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };

    float boundingRadius = 1.0f;
};

struct hierarchy_component
{
    entity parent{ 0 };        ///< 0: Kök (Root) varlık, ebeveyni yoktur.
    entity first_child{ 0 };   ///< İlk çocuğun kimliği.
    entity next_sibling{ 0 };  ///< Aynı ebeveyne bağlı bir sonraki kardeş kimliği.
    entity prev_sibling{ 0 };  ///< Aynı ebeveyne bağlı bir önceki kardeş kimliği (Çift yönlü bağlı liste).
};

struct mesh_component
{
    aria::handle<aria::model_asset> model_handle;
    size_t sub_mesh_index{ 0 };
    mesh_source_type source = mesh_source_type::model;
};

/**
 * @struct bone_transform
 * @brief Tek bir kemiğin yerel TRS (Translation, Rotation, Scale) pozu.
 */
struct bone_transform
{
    glm::vec3 position{ 0.0f };
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f };

    [[nodiscard]] glm::mat4 to_matrix() const
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
        m *= glm::mat4_cast(glm::normalize(rotation));
        m = glm::scale(m, scale);
        return m;
    }
};

/**
 * @struct skinned_mesh_component
 * @brief İskeletli (derili) 3D modellerin render referansını tutar.
 */
struct skinned_mesh_component
{
    aria::handle<aria::skinned_model_asset> model_handle; ///< Model geometrisi (raw_skinned_vertex içeren).
    size_t sub_mesh_index{ 0 };                           ///< Çizilecek alt ağ indeksi.
    bool cast_shadows{ true };                            ///< Gölge haritasına dahil edilme durumu.
};

/**
 * @struct skeleton_component
 * @brief Bir varlığın çalışma zamanındaki canlı kemik pozlarını ve GPU matris paletini barındırır.
 */
struct skeleton_component
{
    aria::handle<aria::skeleton_asset> skeleton_handle;

    /// @brief Her kemiğin ebeveynine göre anlık yerel TRS dönüşümü (Animatör veya IK tarafından yazılır).
    std::vector<bone_transform> local_pose;

    /// @brief Kemiğin aktif animasyonda bir kanalı olup olmadığını belirtir (1: animasyonlu, 0: bind pose kullanılır).
    std::vector<uint8_t> bone_animated;

    /// @brief Her kemiğin model kök uzayındaki global matrisi (Hiyerarşi çözülünce hesaplanır).
    std::vector<glm::mat4> global_pose;

    /// @brief GPU Vertex Shader'a aktarılacak nihai matris paleti: GlobalInverse * GlobalPose * OffsetMatrix.
    std::vector<glm::mat4> skinning_palette;

    bool is_dirty{ true }; ///< Poz değiştiğinde hiyerarşinin yeniden hesaplanmasını tetikler.
};

/**
 * @struct animation_blend_state
 * @brief İki animasyon arasındaki yumuşak geçiş (Crossfade) durum bilgisi.
 */
struct animation_blend_state
{
    aria::handle<aria::animation_clip_asset> target_clip;
    float current_weight{ 0.0f };  ///< 0.0 (mevcut klip) ile 1.0 (hedef klip) arası.
    float target_weight{ 1.0f };
    float fade_duration{ 0.25f };  ///< Geçiş süresi (saniye cinsinden).
    float fade_timer{ 0.0f };
    float target_time{ 0.0f };     ///< Hedef klibin zaman sayacı.
    bool is_blending{ false };
};

/**
 * @struct animator_component
 * @brief İskelete animasyon klibi uygulayan oynatıcı bileşen.
 */
struct animator_component
{
    aria::handle<aria::animation_clip_asset> current_clip;
    float playback_time{ 0.0f };    ///< Klip içindeki mevcut zaman (saniye).
    float playback_speed{ 1.0f };   ///< Oynatma hızı çarpanı (1.0 normal, 0.5 yavaş, -1.0 ters).
    bool is_playing{ true };
    bool is_looping{ true };
    bool apply_root_motion{ false };

    animation_blend_state blend_state;

    void play(aria::handle<aria::animation_clip_asset> clip, bool loop = true)
    {
        current_clip = clip;
        playback_time = 0.0f;
        is_playing = true;
        is_looping = loop;
        blend_state.is_blending = false;
    }

    void stop()
    {
        playback_time = 0.0f;
        is_playing = false;
    }

    void pause()
    {
        is_playing = false;
    }

    void resume()
    {
        is_playing = true;
    }

    void crossfade(aria::handle<aria::animation_clip_asset> targetClip, float duration = 0.25f)
    {
        if (!targetClip.is_valid() || current_clip == targetClip)
        {
            return;
        }

        blend_state.target_clip = targetClip;
        blend_state.current_weight = 0.0f;
        blend_state.target_weight = 1.0f;
        blend_state.fade_duration = duration > 0.0f ? duration : 0.001f;
        blend_state.fade_timer = 0.0f;
        blend_state.target_time = 0.0f;
        blend_state.is_blending = true;
    }
};

/**
 * @struct bone_attachment_component
 * @brief Bir varlığın başka bir varlığın belirli bir kemiğine yapışmasını sağlar (Silah, kalkan vb.).
 */
struct bone_attachment_component
{
    entity target_skeleton_entity{ 0 }; ///< İskelet bileşenine sahip ebeveyn varlık.
    std::string bone_name;              ///< Takip edilecek kemik adı (Örn: "mixamorig:RightHand").
    int cached_bone_index{ -1 };        ///< Hızlı erişim için önbelleklenmiş kemik indeksi.

    glm::vec3 local_offset{ 0.0f };     ///< Kemik eksenine göre yerel konum sapması.
    glm::vec3 local_rotation{ 0.0f };   ///< Kemik eksenine göre yerel açı sapması (Euler açıları).
    glm::vec3 local_scale{ 1.0f };      ///< Yerel ölçek sapması.
};

struct material_component
{
    aria::handle<aria::material_asset> material_handle;
};

struct camera_component
{
    float fov = 45.f;
    float nearPlane = 0.1f;
    float farPlane = 1000.f;

    bool primary = false;   ///< Sistemin bu kamerayı ana kamera olarak kullanıp kullanmayacağını belirler.
    int priority = 0;       ///< İleride çoklu kamera (multi-camera) sıralaması için öncelik değeri.
};

struct light_component
{
    light_type type = light_type::point;

    glm::vec3 color{ 1.0f };
    float intensity = 1.0f;
    float range = 10.0f;

    float inner_cone = 15.0f;
    float outer_cone = 25.0f;

    bool cast_shadows = false;
};

struct name_component
{
    std::string name;
};

struct bounds_component
{
    glm::vec3 localCenter{ 0.0f }; ///< Ağ AABB merkezi, yerel (local) uzayda pivot noktasına göre sapma (offset).
    float localRadius = 0.0f;      ///< localCenter noktasından en uzak köşe (vertex) mesafesi.
};

struct rigidbody_component
{
    motion_type motion = motion_type::dynamic_body;
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.1f; ///< Zıplama/esneklik katsayısı (0.0 = enerjiyi yutar, 1.0 = tam yansıtır).

    bool freeze_rotation_x = false;
    bool freeze_rotation_z = false;
    bool freeze_rotation_y = false;

    /// @brief Jolt Physics dünyasındaki gövde kimliği. Oynatma moduna (PLAY) geçilene kadar geçersizdir (Invalid).
    JPH::BodyID body_id;
};

struct collider_component
{
    collider_type type = collider_type::box;

    /// @brief Kutu (Box) ise genişlik/yükseklik/derinlik yarıçapları (Half-extents). Küre (Sphere) ise 'x' değeri yarıçapı temsil eder.
    glm::vec3 extents{ 1.0f };
};

struct native_script_component
{
    scriptable_entity* instance = nullptr;
    std::string script_name;

    // Çalışma zamanında script nesnesinin bellek döngüsünü (allocation/deallocation) yönetecek köprü fonksiyonlar.
    std::function<scriptable_entity*()> bind_instantiate = nullptr;
    std::function<void(native_script_component*)> bind_destroy = nullptr;

    /**
     * @brief İlgili betik sınıfını varlığa çalışma zamanında bağlar.
     * @tparam T scriptable_entity'den türetilmiş kullanıcı betiği sınıfı.
     * @param name Betiğin kayıtlı ismi.
     */
    template<typename T>
    void bind(const std::string& name)
    {
        script_name = name;
        bind_instantiate = []() { return static_cast<scriptable_entity*>(new T()); };
        bind_destroy = [](native_script_component* nsc) {
            delete static_cast<T*>(nsc->instance);
            nsc->instance = nullptr;
            };
    }
};

// ============================================================================
// SERA YANSIMA (REFLECTION) KAYITLARI
// ============================================================================

DESCRIBE_COMPONENT(name_component,
    SERA_FIELD(name)
)

DESCRIBE_COMPONENT(transform_component,
    SERA_FIELD(position),
    SERA_FIELD(rotation),
    SERA_FIELD(scale),
    SERA_FIELD(boundingRadius)
)

DESCRIBE_COMPONENT(hierarchy_component,
    SERA_FIELD(parent),
    SERA_FIELD(first_child),
    SERA_FIELD(next_sibling),
    SERA_FIELD(prev_sibling)
)

DESCRIBE_COMPONENT(camera_component,
    SERA_FIELD(fov),
    SERA_FIELD(nearPlane),
    SERA_FIELD(farPlane),
    SERA_FIELD(primary),
    SERA_FIELD(priority)
)

DESCRIBE_COMPONENT(light_component,
    SERA_FIELD(type),
    SERA_FIELD(color),
    SERA_FIELD(intensity),
    SERA_FIELD(range),
    SERA_FIELD(inner_cone),
    SERA_FIELD(outer_cone),
    SERA_FIELD(cast_shadows)
)

DESCRIBE_COMPONENT(bounds_component,
    SERA_FIELD(localCenter),
    SERA_FIELD(localRadius)
)

// Not: Jolt BodyID değeri diske kalıcı olarak yazılmaz, sadece fizik ayarları serileştirilir.
DESCRIBE_COMPONENT(rigidbody_component,
    SERA_FIELD(motion),
    SERA_FIELD(mass),
    SERA_FIELD(friction),
    SERA_FIELD(restitution),
    SERA_FIELD(freeze_rotation_x),
    SERA_FIELD(freeze_rotation_y),
    SERA_FIELD(freeze_rotation_z)
)

DESCRIBE_COMPONENT(collider_component,
    SERA_FIELD(type),
    SERA_FIELD(extents)
)

DESCRIBE_COMPONENT(native_script_component,
    SERA_FIELD(script_name)
)

DESCRIBE_COMPONENT(skinned_mesh_component,
    SERA_FIELD(sub_mesh_index),
    SERA_FIELD(cast_shadows)
)

DESCRIBE_COMPONENT(skeleton_component,
    SERA_FIELD(is_dirty)
)

DESCRIBE_COMPONENT(animator_component,
    SERA_FIELD(playback_speed),
    SERA_FIELD(is_playing),
    SERA_FIELD(is_looping),
    SERA_FIELD(apply_root_motion)
)

DESCRIBE_COMPONENT(bone_attachment_component,
    SERA_FIELD(target_skeleton_entity),
    SERA_FIELD(bone_name),
    SERA_FIELD(local_offset),
    SERA_FIELD(local_rotation),
    SERA_FIELD(local_scale)
)

DESCRIBE_COMPONENT(canvas_component,
    SERA_FIELD(render_mode),
    SERA_FIELD(scaler_mode),
    SERA_FIELD(reference_resolution),
    SERA_FIELD(match_width_or_height),
    SERA_FIELD(sorting_order)
)

DESCRIBE_COMPONENT(rect_transform_component,
    SERA_FIELD(anchored_position),
    SERA_FIELD(size),
    SERA_FIELD(anchor_min),
    SERA_FIELD(anchor_max),
    SERA_FIELD(pivot),
    SERA_FIELD(rotation),
    SERA_FIELD(scale),
    SERA_FIELD(visible)
)

DESCRIBE_COMPONENT(ui_text_component,
    SERA_FIELD(text),
    SERA_FIELD(font_path),
    SERA_FIELD(font_size),
    SERA_FIELD(color),
    SERA_FIELD(alignment),
    SERA_FIELD(wrap_text),
    SERA_FIELD(line_spacing)
)

DESCRIBE_COMPONENT(ui_image_component,
    SERA_FIELD(color),
    SERA_FIELD(raycast_target)
)

DESCRIBE_COMPONENT(ui_button_component,
    SERA_FIELD(interactable),
    SERA_FIELD(normal_color),
    SERA_FIELD(hover_color),
    SERA_FIELD(pressed_color),
    SERA_FIELD(disabled_color)
)

DESCRIBE_COMPONENT(audio_source_component,
    SERA_FIELD(clip_path),
    SERA_FIELD(volume),
    SERA_FIELD(pitch),
    SERA_FIELD(loop),
    SERA_FIELD(play_on_awake),
    SERA_FIELD(is_3d),
    SERA_FIELD(min_distance),
    SERA_FIELD(max_distance),
    SERA_FIELD(roll_off)
)

DESCRIBE_COMPONENT(audio_listener_component,
    SERA_FIELD(is_active)
)