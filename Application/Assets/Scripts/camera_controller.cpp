#include "camera_controller.h"
#include "../Engine/engine_context.h"
#include "../ECS/components.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp> // glm::pi için gerekli
#include <algorithm>
#include <iostream>
#include <cmath> 

void camera_controller::on_create()
{
    target = find_by_name("Player");
}

void camera_controller::on_update(float dt)
{
    auto* reg = get_registry();
    auto& input = getContext().inputState;

    auto* playerT = reg->get_component<transform_component>(target);

    if (!playerT)
        return;

    auto& camT = get_component<transform_component>();

    // Eğer sensitivity değerin önceden dereceye göreyse, radyan dünyasında çok hızlı gelebilir.
    // Gerekirse hassasiyeti biraz düşürebilirsin (örn: 0.002f gibi)
    camT.rotation.x -= input.gameMouseDelta.y * sensitivity;
    camT.rotation.y += input.gameMouseDelta.x * sensitivity;

    // 1. DÜZELTME: YAW (Sağ-Sol) Radian Wrap (0 ile 2*PI arası)
    // 360 derece = 2 * PI radyandır.
    const float twoPi = 2.0f * glm::pi<float>();
    camT.rotation.y = std::fmod(camT.rotation.y, twoPi);
    if (camT.rotation.y < 0.0f) {
        camT.rotation.y += twoPi;
    }

    // 2. DÜZELTME: PITCH (Yukarı-Aşağı) Radian Clamp
    // 90 derece = PI / 2 radyandır (yaklaşık 1.57). 
    // Gimbal lock yememek için güvenli sınır olarak 85 derece karşılığı olan 1.48f radyanı kullanıyoruz.
    const float maxPitchRadian = 1.48f;
    camT.rotation.x = std::clamp(camT.rotation.x, -maxPitchRadian, maxPitchRadian);

    // FPS kamera oyuncunun gözünde
    camT.position = playerT->position + glm::vec3(0.0f, 3.7f, 0.0f);
}