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
    // 1. Kamera varlığını kendi kayıt defteri (registry) üzerinden güvenle bul
    const entity camId = find_by_name("MainCamera");
    if (camId == 0)
    {
        return;
    }

    auto camEntity = gameplay::gameplay_entity(camId, get_registry());

    // 2. Kameranın yatay bakış açısını (Yaw) alarak hareket doğrultularını hesapla
    const float yaw = camEntity.transform().get_rotation_euler().y;

    // Standart OpenGL sağ el kuralı koordinat sisteminde yönler (ileri = -Z)
    const glm::vec3 forward = glm::normalize(glm::vec3(std::sin(yaw), 0.0f, -std::cos(yaw)));
    const glm::vec3 right = glm::normalize(glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw)));

    // 3. Gameplay Input API üzerinden 2D eksen hareket girdisini al
    const glm::vec2 axis = gameplay::input::get_axis_2d(
        InputAction::MoveForward, InputAction::MoveBackward,
        InputAction::MoveRight,   InputAction::MoveLeft
    );

    glm::vec3 moveDir = forward * axis.y + right * axis.x;
    if (glm::length(moveDir) > 0.001f)
    {
        moveDir = glm::normalize(moveDir);
    }

    // 4. Mevcut Y ekseni hızını (yerçekimi ve dikey ivme) koruyarak yatay hızı ata
    const glm::vec3 currentVel = rigidbody().get_velocity();
    const glm::vec3 finalVel(moveDir.x * speed, currentVel.y, moveDir.z * speed);

    rigidbody().set_velocity(finalVel);
}