#pragma once
#include <glm/glm.hpp>
#include <string>

//Contains all components that can be attached to an entity

struct transform_component {
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };
};

struct mesh_component {
    unsigned int mesh_id;
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
    glm::vec3 color = glm::vec3(1.0f);
    float     intensity = 1.0f;
};
struct name_component {
    std::string name;
};