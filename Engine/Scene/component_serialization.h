#pragma once
#include "../Externals/nlohmann/json.hpp"
#include "../ECS/components.h"
#include <glm/glm.hpp>
#include "../ECS/entity.h"
#include "../ECS/registry.h"

// ============================================================================
// 1. GLM VEC2/VEC3 İÇİN GÜVENLİ DÖNÜŞÜM KURALLARI
// ============================================================================
namespace glm {
    inline void to_json(nlohmann::json& j, const vec3& v) {
        j = nlohmann::json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
    }

    inline void to_json(nlohmann::json& j, const vec2& v) {
        j = nlohmann::json{ {"x", v.x}, {"y", v.y} };
    }

    inline void from_json(const nlohmann::json& j, vec3& v) {
        try {
            v.x = j.value("x", 0.0f);
            v.y = j.value("y", 0.0f);
            v.z = j.value("z", 0.0f);
        }
        catch (...) {
            v = vec3(0.0f);
        }
    }

    inline void from_json(const nlohmann::json& j, vec2& v) {
        try {
            v.x = j.value("x", 0.0f);
            v.y = j.value("y", 0.0f);
        }
        catch (...) {
            v = vec2(0.0f);
        }
    }
}

// ============================================================================
// 2. ENUM DÖNÜŞÜMLERİ
// ============================================================================
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

// ============================================================================
// 3. TÜM BİLEŞENLERİN DOĞRUDAN JSON NESNESİNE EŞLENMESİ (Makro Sihri)
// ============================================================================
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material, shader_id, base_color, diffuse_texture, normal_texture, specular_texture, normal_strength, texture_scale)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(name_component, name)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(transform_component, position, rotation, scale, boundingRadius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(mesh_component, mesh_id, model_path)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material_component, material_id)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(camera_component, fov, nearPlane, farPlane, primary, priority)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(light_component, type, color, intensity, range, inner_cone, outer_cone, cast_shadows)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(bounds_component, localCenter, localRadius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(rigidbody_component, motion, mass, friction, restitution)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(collider_component, type, extents)

// NATIVE SCRIPT COMPONENT İÇİN ÖZEL DÖNÜŞÜM KURALLARI
inline void to_json(nlohmann::json& j, const native_script_component& nsc) {
    j = nlohmann::json{ {"script_name", nsc.script_name} };
}

inline void from_json(const nlohmann::json& j, native_script_component& nsc) {
    nsc.script_name = j.value("script_name", "None");
   // Dikkat: Instance bağlama (bind) işlemi burada değil, sahne yüklenirken fabrikada yapılacak!
}

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

// 🛠️ Bir entity'de bu bileşen varsa JSON'a yazan jenerik yardımcı fonksiyon
template <typename T>
void serialize_component_if_exists(entity e, const registry& reg, nlohmann::json& entityJson, const std::string& keyName)
{
    if (reg.has_component<T>(e))
    {
        // Makro sayesinde otomatik json'a dönüşüyor
        entityJson[keyName] = *reg.get_component<T>(e);
    }
}

// 🛠️ JSON içinde bu bileşen başlığı varsa diskten okur ve entity'ye jenerik olarak ekler
template <typename T>
void deserialize_component_if_exists(entity e, registry& reg, const nlohmann::json& entityJson, const std::string& keyName)
{
    if (entityJson.contains(keyName))
    {
        // Makromuz sayesinde otomatik JSON'dan struct'a dönüşüm (implicit conversion) tetiklenir
        T comp = entityJson[keyName].get<T>();
        reg.add_component<T>(e, comp);
    }
}