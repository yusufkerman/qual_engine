#pragma once

/**
 * @file camera_controller.h
 * @brief Gameplay API tabanlı FPS kamera takip ve yönelim betiği.
 */

#include <Gameplay/gameplay.h>
#include <glm/glm.hpp>

/**
 * @class camera_controller
 * @brief Oyuncunun konumunu takip eden ve fare hareketine göre dönen kamera kontrolcüsü.
 */
class camera_controller : public gameplay::script
{
public:
    void on_create() override;
    void on_update(float dt) override;

private:
    entity target = 0;
    float sensitivity = 0.005f;
};