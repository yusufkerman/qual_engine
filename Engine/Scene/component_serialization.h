#pragma once

#include "../Externals/nlohmann/json.hpp"
#include "../ECS/components.h"
#include "../ECS/entity.h"
#include "../ECS/registry.h"
#include <glm/glm.hpp>

using json = nlohmann::json;

// ======================================================
// 1. GLM SERIALIZATION (SADECE MATH, COMPONENT YOK)
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
// 2. ENUM SERIALIZATION
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
    // 3. COMPONENT SERIALIZATION
    // ======================================================
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material,
        shader_id, base_color,
        diffuse_texture, normal_texture,
        specular_texture, normal_strength,
        texture_scale)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(name_component, name)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(transform_component,
        position, rotation, scale, boundingRadius)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(mesh_component,
        mesh_id, model_path, source)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material_component, material_id)
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
// 4. NATIVE SCRIPT
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
// 5. ENGINE HELPERS
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
// 6. GENERIC SERIALIZER
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