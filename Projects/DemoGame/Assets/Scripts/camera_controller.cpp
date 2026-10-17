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
    gameplay::input::set_cursor_locked(true);
}

void camera_controller::on_destroy()
{
    gameplay::input::set_cursor_locked(false);
}

void camera_controller::on_update(float dt)
{
    // 0. İmleç kontrolü: ESC ile imleci serbest bırak, Space/Enter ile tekrar kilitle
    if (gameplay::input::is_key_just_pressed(256)) // GLFW_KEY_ESCAPE
    {
        gameplay::input::set_cursor_locked(false);
    }
    else if (gameplay::input::is_key_just_pressed(32) || gameplay::input::is_key_just_pressed(257)) // Space or Enter
    {
        gameplay::input::set_cursor_locked(true);
    }
    // 1. Hedef oyuncu kimliğini doğrula / gerekirse tekrar bul
    if (target == 0)
    {
        target = find_by_name("Player");
        if (target == 0)
        {
            target = find_by_name("player");
        }
        if (target == 0)
        {
            auto* reg = get_registry();
            if (reg)
            {
                for (auto& [e, nsc] : reg->get_all<native_script_component>())
                {
                    if (nsc.script_name == "player_controller" && e != entity_id())
                    {
                        target = e;
                        break;
                    }
                }
            }
        }
    }

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

    // 6. Kamerayı oyuncunun baş/göz hizasına taşı (Eğer bu script yanlışlıkla Player'ın kendisine takılmışsa kendi kendini yukarı fırlatmasını önle)
    if (target != 0 && target != entity_id())
    {
        gameplay::transform_wrapper targetTransform(target, get_registry());
        transform().set_position(targetTransform.get_position() + glm::vec3(0.0f, 3.7f, 0.0f));
    }
}