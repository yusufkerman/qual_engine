#pragma once

/**
 * @file player_controller.h
 * @brief Gameplay API tabanlı oyuncu hareket kontrol betiği.
 */

#include <Gameplay/gameplay.h>
#include <glm/glm.hpp>

/**
 * @class player_controller
 * @brief Oyuncunun fiziksel hareketlerini ve girdi etkileşimlerini yöneten kontrol betiği.
 */
class player_controller : public gameplay::script
{
public:
    void on_create() override;
    void on_update(float dt) override;

private:
    float speed = 5.0f;
};