#pragma once
#include <Scene/scriptable_entity.h>
#include <Engine/Scripting/script_factory.h>
#include <glm/glm.hpp>

class player_controller : public scriptable_entity
{
public:
    void on_create() override;
    void on_update(float dt) override;

private:
    float speed = 5.0f;
};

REGISTER_SCRIPT(player_controller);