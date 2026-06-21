#pragma once
#include "../Externals/nlohmann/json.hpp"
#include "../ECS/components.h"
#include <glm/glm.hpp>
#include "../ECS/entity.h"
#include "../ECS/registry.h"

// ============================================================================
// 1. GLM VEC3 İÇİN DOĞRUDAN DÖNÜŞÜM KURALLARI (Net ve Düz)
// ============================================================================
namespace glm {
    inline void to_json(nlohmann::json& j, const vec3& v) {
        j = nlohmann::json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
    }

    // 🌟 Vec2 Kaydetme Kuralları
    inline void to_json(nlohmann::json& j, const vec2& v) {
        j = nlohmann::json{ {"x", v.x}, {"y", v.y} };
    }

    inline void from_json(const nlohmann::json& j, vec3& v) {
        v.x = j.at("x").get<float>();
        v.y = j.at("y").get<float>();
        v.z = j.at("z").get<float>();
    }

    
    inline void from_json(const nlohmann::json& j, vec2& v) {
        v.x = j.at("x").get<float>();
        v.y = j.at("y").get<float>();
    }
}

// ============================================================================
// 2. ENUM (LIGHT TYPE) DÖNÜŞÜMÜ
// ============================================================================
NLOHMANN_JSON_SERIALIZE_ENUM(light_type, {
    {light_type::directional, "directional"},
    {light_type::point, "point"},
    {light_type::spot, "spot"}
    })

// ============================================================================
// 3. TÜM BİLEŞENLERİN DOĞRUDAN JSON NESNESİNE EŞLENMESİ (Makro Sihri)
// ============================================================================
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(name_component, name)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(transform_component, position, rotation, scale, boundingRadius)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(mesh_component, mesh_id, model_path)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material_component, material_id)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(camera_component, fov, nearPlane, farPlane, primary, priority)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(light_component, type, color, intensity, range, inner_cone, outer_cone, cast_shadows)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(bounds_component, localCenter, localRadius)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(material, shader_id, base_color, diffuse_texture, normal_texture, specular_texture, normal_strength, texture_scale)


using EngineComponentList = std::tuple<
    name_component,
    transform_component,
    mesh_component,
    material_component,
    camera_component,
    light_component,
    bounds_component
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