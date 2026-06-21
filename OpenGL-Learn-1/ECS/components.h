#pragma once
#include <glm/glm.hpp>
#include <string>

//Contains all components that can be attached to an entity

//Light type for light component.
enum class light_type
{
    directional,
    point,
    spot
};

struct transform_component {
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };

    float boundingRadius = 1.0f;

};

struct mesh_component {
    unsigned int mesh_id;
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
