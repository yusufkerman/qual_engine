#pragma once
#include <Scene/scriptable_entity.h>
#include <Engine/Scripting/script_factory.h>
#include <glm/glm.hpp>

class camera_controller : public scriptable_entity
{
public:
    void on_create() override;
    void on_update(float dt) override;

private:
    entity target = 0;

    float yaw = -90.0f;
    float pitch = 0.0f;

    glm::vec3 forward{ 0.0f, 0.0f, -1.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };

    glm::vec3 offset{ 0.0f, 3.5f, -6.0f };

    float sensitivity = 0.005f;
    float smooth = 10.0f;
};

REGISTER_SCRIPT(camera_controller);