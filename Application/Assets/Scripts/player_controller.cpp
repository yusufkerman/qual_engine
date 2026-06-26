#include "player_controller.h"
#include "../Engine/engine_context.h"
#include "../ECS/components.h"
#include "../Systems/physics_api.h"
#include "../Systems/physics_system.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <iostream>

static glm::vec3 getForward(const glm::vec3& rot)
{
    float yaw = rot.y;

    return glm::normalize(glm::vec3(
        sin(yaw),
        0.0f,
        cos(yaw)
    ));
}

static glm::vec3 getRight(const glm::vec3& rot)
{
    float yaw = rot.y;

    return glm::normalize(glm::vec3(
        cos(yaw),
        0.0f,
        -sin(yaw)
    ));
}

void player_controller::on_create()
{
    getContext().inputState.cursorLocked = true;
}

void player_controller::on_update(float dt)
{
    auto& input = getContext().inputState;
    auto* reg = get_registry();

    auto* camT =
        reg->get_component<transform_component>(
            find_by_name("MainCamera"));

    if (!camT)
        return;

    // SADECE Y (Yaw) eksenini alıyoruz. Pitch (yukarı/aşağı) hareketimizi etkilememeli.
    // NOT: Eğer rotasyon değerlerin derece (degree) cinsindense, burayı glm::radians(camT->rotation.y) yapmalısın.
    float yaw = camT->rotation.y;

    // Standart OpenGL (Right-Handed) koordinat sisteminde ileri yön -Z'dir.
    // Eğer 'W'ye bastığında geriye gidiyorsan, -cos(yaw) yerine +cos(yaw) kullanabilirsin.
    glm::vec3 forward = glm::vec3(sin(yaw), 0.0f, -cos(yaw));
    glm::vec3 right = glm::vec3(cos(yaw), 0.0f, sin(yaw));

    // Formül gereği genelde 1 uzunluğundadır ama garantiye alıyoruz
    forward = glm::normalize(forward);
    right = glm::normalize(right);

    glm::vec3 moveDir(0.0f);

    if (input.actions[(int)InputAction::MoveForward])
        moveDir += forward;

    if (input.actions[(int)InputAction::MoveBackward])
        moveDir -= forward;

    if (input.actions[(int)InputAction::MoveLeft])
        moveDir -= right;

    if (input.actions[(int)InputAction::MoveRight])
        moveDir += right;

    // Birden fazla tuşa basıldığında (çapraz gidiş) hızın artmasını engellemek için normalize ediyoruz
    if (glm::length(moveDir) > 0.001f)
        moveDir = glm::normalize(moveDir);

    /* --- Log ve Fizik Kısmı --- */

    // (Opsiyonel) Log atıp değerleri kontrol edebilirsin
    // std::cout << "MOVE: " << moveDir.x << " " << moveDir.y << " " << moveDir.z << std::endl;

    auto& physics = getContext().physicsSystem.api();

    // Mevcut Y hızını (yerçekimi/zıplama) koru
    glm::vec3 currentVel = physics.get_velocity(get_entity());
    glm::vec3 horizontalVel(0.0f);

    if (glm::length(moveDir) > 0.001f)
    {
        horizontalVel = moveDir * speed;
    }

    // Y eksenindeki fizik tepkimelerini bozmadan yeni hızı oluştur
    glm::vec3 finalVel;
    finalVel.x = horizontalVel.x;
    finalVel.y = currentVel.y; // Yerçekimi bozulmaz
    finalVel.z = horizontalVel.z;

    physics.set_velocity(get_entity(), finalVel);
}