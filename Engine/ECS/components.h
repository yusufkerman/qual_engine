#pragma once

/**
 * @file components.h
 * @brief Motor içerisindeki tüm standart ECS bileşen yapılarını (struct) ve SERA yansıma kayıtlarını barındırır.
 */

#include <glm/glm.hpp>
#include <string>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include "../Engine/Assets/aria.h"
#include "../Sera/sera_component_codec.h"

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

// ============================================================================
// BİLEŞENLER (COMPONENTS)
// ============================================================================

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
    scriptable_entity* (*bind_instantiate)() = nullptr;
    void (*bind_destroy)(native_script_component*) = nullptr;

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