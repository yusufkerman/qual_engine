#pragma once
#include <glm/glm.hpp>

//Contains all components that can be attached to an entity

struct transform_component {
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };
};

struct mesh_component {
    unsigned int mesh_id;
};

struct camera_component {
    float fov;
    float nearPlane;
    float farPlane;
};

struct light_component
{
    glm::vec3 color = glm::vec3(1.0f);
    float     intensity = 1.0f;
};