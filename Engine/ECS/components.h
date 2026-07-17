#pragma once
#include <glm/glm.hpp>
#include <string>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include "../Engine/Assets/aria.h"
#include "../Sera/sera_component_codec.h"

class scriptable_entity;

//Contains all components that can be attached to an entity

//Light type for light component.
enum class light_type
{
    directional,
    point,
    spot
};

// Fizik nesnesinin türü (Statik: Harita elementleri, Dinamik: Düşen taşlar, Kinematik: Kodla hareket eden platformlar)
enum class motion_type
{
    static_body,
    kinematic_body,
    dynamic_body
    
};

// Çarpışma kutularının şekli
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


struct transform_component {
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };

    float boundingRadius = 1.0f;

};

struct hierarchy_component {
    entity parent{ 0 };        // 0 = Root (Ebeveyni yok)
    entity first_child{ 0 };  // İlk çocuğun ID'si
    entity next_sibling{ 0 }; // Aynı parent'a bağlı sıradaki kardeşin ID'si
    entity prev_sibling{ 0 }; // Bir önceki kardeşin ID'si (Çift yönlü bağlı liste - kolay silme için)
};

//struct mesh_component {
//    unsigned int mesh_id;
//
//    std::string model_path = "None";   // Inspector'da hangi dosyanın yüklü olduğunu görmek için
//
//    mesh_source_type source = mesh_source_type::model;
//};
struct mesh_component {
    aria::handle<aria::model_asset> model_handle;
    size_t sub_mesh_index{ 0 };

    mesh_source_type source = mesh_source_type::model;
};

//struct material_component
//{
//    unsigned int material_id = 0;
//};
struct material_component {
    aria::handle<aria::material_asset> material_handle; // Artık handle kullanıyoruz
};

struct camera_component
{
    float fov = 45.f;
    float nearPlane = 0.1f;
    float farPlane = 1000.f;

    bool primary = false;   // kullanıcı işaretler
    int priority = 0;       // ileride multi camera için
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
struct name_component {
    std::string name;
};
struct bounds_component
{
    glm::vec3 localCenter{ 0.0f };   // mesh AABB merkezi, LOCAL space, pivot'a göre offset
    float localRadius = 0.0f;      // localCenter'dan en uzak vertex'e mesafe
};

//Physics
struct rigidbody_component
{
    motion_type motion = motion_type::dynamic_body;
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.1f; // Zıplama/esneklik katsayısı (0 = hiç zıplamaz, 1 = tam zıplar)

    // Rotasyon kilitli mi?
    bool freeze_rotation_x = false;
    bool freeze_rotation_z = false;
    bool freeze_rotation_y = false;

    // Jolt tarafındaki karşılığı. Simülasyon başlamadan önce (EDITOR modunda) geçersizdir.
    // PLAY moduna girildiğinde Jolt burayı dolduracak.
    JPH::BodyID body_id;
};

struct collider_component
{
    collider_type type = collider_type::box;

    // Box ise genişlik/yükseklik/derinlik yarıçapları (Half-extents)
    // Sphere ise x değeri radius (yarıçap) olarak kullanılır
    glm::vec3 extents{ 1.0f };
};

//Scritable component
struct native_script_component {

	scriptable_entity* instance = nullptr;
    std::string script_name;
    
    // Çalışma zamanında (Runtime) script nesnesini RAM'de "new"lemek ve "delete"lemek için 
    // fonksiyon pointer'ları saklıyoruz. Komponent fonksiyon çalıştırmıyor, sadece adres tutuyor.
    scriptable_entity* (*bind_instantiate)() = nullptr;
    void (*bind_destroy)(native_script_component*) = nullptr;

    // Kullanıcının Sandbox'ta kolayca script bağlayabilmesi için şablon fonksiyon
    template<typename T>
    void bind(const std::string& name)
    {
        script_name = name;
        bind_instantiate = []() { return static_cast<scriptable_entity*>(new T()); };
        bind_destroy = [](native_script_component* nsc) { delete (T*)nsc->instance; nsc->instance = nullptr; };
    }
};

// ============================================================================
// 🔥 SERA REFLECTION REGISTRATION FOR ENGINE COMPONENTS
// ============================================================================

// 1. Name Component
DESCRIBE_COMPONENT(name_component,
    SERA_FIELD(name)
)

// 2. Transform Component
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
// 3. Camera Component
DESCRIBE_COMPONENT(camera_component,
    SERA_FIELD(fov),
    SERA_FIELD(nearPlane),
    SERA_FIELD(farPlane),
    SERA_FIELD(primary),
    SERA_FIELD(priority)
)

// 4. Light Component
DESCRIBE_COMPONENT(light_component,
    SERA_FIELD(type), // enum sınıfları integer olarak nlohmann::json tarafından otomatik serileştirilir
    SERA_FIELD(color),
    SERA_FIELD(intensity),
    SERA_FIELD(range),
    SERA_FIELD(inner_cone),
    SERA_FIELD(outer_cone),
    SERA_FIELD(cast_shadows)
)

// 5. Bounds Component
DESCRIBE_COMPONENT(bounds_component,
    SERA_FIELD(localCenter),
    SERA_FIELD(localRadius)
)

// 6. Rigidbody Component (Jolt BodyID diske yazılmaz, sadece ayarlar kaydedilir)
DESCRIBE_COMPONENT(rigidbody_component,
    SERA_FIELD(motion),
    SERA_FIELD(mass),
    SERA_FIELD(friction),
    SERA_FIELD(restitution),
    SERA_FIELD(freeze_rotation_x),
    SERA_FIELD(freeze_rotation_y),
    SERA_FIELD(freeze_rotation_z)
)

// 7. Collider Component
DESCRIBE_COMPONENT(collider_component,
    SERA_FIELD(type),
    SERA_FIELD(extents)
)

// 8. Native Script Component (Sadece yüklü olan script'in adı kaydedilir)
DESCRIBE_COMPONENT(native_script_component,
    SERA_FIELD(script_name)
)