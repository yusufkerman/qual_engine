/**
 * @file player_controller.cpp
 * @brief Gameplay API tabanlı oyuncu kontrol uygulaması.
 */

#include "player_controller.h"
#include <glm/glm.hpp>
#include <cmath>

void player_controller::on_create()
{
}

void player_controller::on_update(float dt)
{
    // 1. Kamera varlığını kendi kayıt defteri (registry) üzerinden esnekçe bul
    entity camId = find_by_name("MainCamera");
    if (camId == 0)
    {
        camId = find_by_name("Main Camera");
    }
    if (camId == 0)
    {
        auto* reg = get_registry();
        if (reg)
        {
            for (auto& [e, cam] : reg->get_all<camera_component>())
            {
                camId = e;
                break;
            }
        }
    }

    glm::vec3 forward(0.0f, 0.0f, -1.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);

    if (camId != 0)
    {
        auto camEntity = gameplay::gameplay_entity(camId, get_registry());
        const float yaw = camEntity.transform().get_rotation_euler().y;
        forward = glm::normalize(glm::vec3(std::sin(yaw), 0.0f, -std::cos(yaw)));
        right = glm::normalize(glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw)));
    }

    // 2. Gameplay Input API üzerinden 2D eksen hareket girdisini al
    const glm::vec2 axis = gameplay::input::get_axis_2d(
        InputAction::MoveForward, InputAction::MoveBackward,
        InputAction::MoveRight,   InputAction::MoveLeft
    );

    glm::vec3 moveDir = forward * axis.y + right * axis.x;
    if (glm::length(moveDir) > 0.001f)
    {
        moveDir = glm::normalize(moveDir);
    }

    // 3. Fizik gövdesi (Rigidbody) varsa fizik hızı ata, yoksa doğrudan transform taşı
    if (rigidbody().is_valid())
    {
        const glm::vec3 currentVel = rigidbody().get_velocity();
        const glm::vec3 finalVel(moveDir.x * speed, currentVel.y, moveDir.z * speed);
        rigidbody().set_velocity(finalVel);
    }
    else
    {
        transform().translate(moveDir * speed * dt);
    }
}