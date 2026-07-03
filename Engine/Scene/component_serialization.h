#pragma once

#include "../Externals/nlohmann/json.hpp"
#include "../ECS/entity.h"
#include "../ECS/registry.h"
#include <glm/glm.hpp>
#include "../Externals/uuid/uuid.h"
#include "../Renderer/material.h"

using json = nlohmann::json;

// ======================================================
// 🔥 0. FORWARD DECLARATIONS (KRİTİK HATA ÇÖZÜCÜ)
// ======================================================
// Derleyici components.h yükleme sırasından dolayı material ve componentleri 
// göremezse diye burada ön bildirim yapıyoruz.
struct material;
struct name_component;
struct transform_component;
struct mesh_component;
struct material_component;
struct camera_component;
struct light_component;
struct bounds_component;
struct rigidbody_component;
struct collider_component;
struct native_script_component;

// Aria namespace'ini ve handle yapısını da önceden bildiriyoruz
namespace aria {
    template<typename T> struct handle;
}

// Şimdi asıl components.h dosyasını güvenle çağırabiliriz
#include "../ECS/components.h"

// ======================================================
// 1. KÖK NLOHMANN NAMESPACE ENJEKSİYONU (ŞABLON TAM TANIMI)
// ======================================================
namespace nlohmann {
    template <typename T>
    struct adl_serializer<aria::handle<T>> {
        static void to_json(json& j, const aria::handle<T>& h) {
            j = json{ {"index", h.index}, {"generation", h.generation} };
        }

        static void from_json(const json& j, aria::handle<T>& h) {
            h.index = j.value("index", 0u);
            h.generation = j.value("generation", 0u);
        }
    };
}

// ======================================================
// 2. GLM SERIALIZATION (SADECE MATH, COMPONENT YOK)
// ======================================================
namespace glm {

    inline void to_json(json& j, const vec3& v)
    {
        j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
    }

    inline void from_json(const json& j, vec3& v)
    {
        v.x = j.value("x", 0.0f);
        v.y = j.value("y", 0.0f);
        v.z = j.value("z", 0.0f);
    }

    inline void to_json(json& j, const vec2& v)
    {
        j = json{ {"x", v.x}, {"y", v.y} };
    }

    inline void from_json(const json& j, vec2& v)
    {
        v.x = j.value("x", 0.0f);
        v.y = j.value("y", 0.0f);
    }

} // glm

// ======================================================
// 3. ENUM SERIALIZATION
// ======================================================
NLOHMANN_JSON_SERIALIZE_ENUM(light_type, {
    {light_type::directional, "directional"},
    {light_type::point, "point"},
    {light_type::spot, "spot"}
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(motion_type, {
        {motion_type::static_body, "static"},
        {motion_type::dynamic_body, "dynamic"},
        {motion_type::kinematic_body, "kinematic"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(collider_type, {
        {collider_type::box, "box"},
        {collider_type::sphere, "sphere"}
        })

    NLOHMANN_JSON_SERIALIZE_ENUM(mesh_source_type, {
        {mesh_source_type::model, "model"},
        {mesh_source_type::primitive_cube, "cube"},
        {mesh_source_type::primitive_sphere, "sphere"}
        })

    // ======================================================
    // 4. COMPONENT SERIALIZATION
    // ======================================================
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material,
        shader_id, base_color,
        diffuse_texture, normal_texture,
        specular_texture, normal_strength,
        texture_scale,
        specular_strength, shininess)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(name_component, name)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(transform_component,
        position, rotation, scale, boundingRadius)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(mesh_component, 
        model_handle, sub_mesh_index)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material_component, material_handle)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(camera_component,
        fov, nearPlane, farPlane, primary, priority)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(light_component,
        type, color, intensity, range,
        inner_cone, outer_cone, cast_shadows)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(bounds_component,
        localCenter, localRadius)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(rigidbody_component,
        motion, mass, friction, restitution)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(collider_component,
        type, extents)

    // ======================================================
    // 5. NATIVE SCRIPT
    // ======================================================
    inline void to_json(json& j, const native_script_component& nsc)
{
    j = json{ {"script_name", nsc.script_name} };
}

inline void from_json(const json& j, native_script_component& nsc)
{
    nsc.script_name = j.value("script_name", "None");
}

// ======================================================
// 6. ENGINE HELPERS
// ======================================================
using EngineComponentList = std::tuple<
    name_component,
    transform_component,
    mesh_component,
    material_component,
    camera_component,
    light_component,
    bounds_component,
    rigidbody_component,
    collider_component
>;

// ======================================================
// 7. GENERIC SERIALIZER
// ======================================================
template <typename T>
void serialize_component_if_exists(entity e, const registry& reg,
    json& entityJson, const std::string& key)
{
    if (reg.has_component<T>(e))
        entityJson[key] = *reg.get_component<T>(e);
}

template <typename T>
void deserialize_component_if_exists(entity e, registry& reg,
    const json& entityJson, const std::string& key)
{
    if (!entityJson.contains(key)) return;

    T comp = entityJson.at(key).get<T>();
    reg.add_component<T>(e, comp);
}