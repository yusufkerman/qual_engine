#pragma once

#include <glm/glm.hpp>
#include "../ECS/entity.h"
#include "../ECS//registry.h"

class physics_system;

class physics_api
{
public:
    physics_api(physics_system* phys, registry* reg);

    void set_velocity(entity e, const glm::vec3& v);
    glm::vec3 get_velocity(entity e);

    void move_kinematic(entity e, const glm::vec3& targetPos, float dt);
    

private:
    physics_system* m_phys;
    registry* m_reg;
};