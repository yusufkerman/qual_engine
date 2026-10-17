/**
 * @file camera_controller.cpp
 * @brief Gameplay API tabanlı FPS kamera kontrol uygulaması.
 */

#include "camera_controller.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

void camera_controller::on_create()
{
    target = find_by_name("Player");
}

void camera_controller::on_update(float dt)
{
    // 1. Hedef oyuncu kimliğini doğrula / gerekirse tekrar bul
    if (target == 0)
    {
        target = find_by_name("Player");
        if (target == 0)
        {
            return;
        }
    }

    auto playerEntity = gameplay::gameplay_entity(target, get_registry());

    // 2. Gameplay Input API üzerinden fare hareket deltasını oku
    const glm::vec2 mouseDelta = gameplay::input::get_mouse_delta();
    glm::vec3 rot = transform().get_rotation_euler();

    rot.x -= mouseDelta.y * sensitivity;
    rot.y += mouseDelta.x * sensitivity;

    // 3. YAW (Sağ-Sol) Radian Wrap (0 ile 2*PI arası)
    const float twoPi = 2.0f * glm::pi<float>();
    rot.y = std::fmod(rot.y, twoPi);
    if (rot.y < 0.0f)
    {
        rot.y += twoPi;
    }

    // 4. PITCH (Yukarı-Aşağı) Clamp (-85 ile +85 derece / 1.48 radyan)
    constexpr float maxPitchRadian = 1.48f;
    rot.x = std::clamp(rot.x, -maxPitchRadian, maxPitchRadian);

    // 5. Güncellenen dönüşüm değerlerini transform sarmalayıcısına yaz
    transform().set_rotation_euler(rot);

    // 6. Kamerayı oyuncunun baş/göz hizasına taşı
    transform().set_position(playerEntity.transform().get_position() + glm::vec3(0.0f, 3.7f, 0.0f));
}