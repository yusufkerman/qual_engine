#pragma once
#include <glm/glm.hpp>
#include <string>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

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
    dynamic_body,
    kinematic_body
};

// Çarpışma kutularının şekli
enum class collider_type
{
    box,
    sphere
};

struct transform_component {
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };

    float boundingRadius = 1.0f;

};

struct mesh_component {
    unsigned int mesh_id;

    std::string model_path = "None";   // Inspector'da hangi dosyanın yüklü olduğunu görmek için
};

struct material_component
{
    unsigned int material_id = 0;
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
    motion_type motion = motion_type::static_body;
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