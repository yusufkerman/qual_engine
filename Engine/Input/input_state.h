#pragma once

/**
 * @file input_state.h
 * @brief Anlık ve bir önceki kareye ait klavye, fare ve eylem durumlarını tutan veri yapısı.
 */

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "input_action.h"

 /**
  * @struct InputState
  * @brief Giriş aygıtlarının anlık donanımsal ve mantıksal durumlarını taşıyan veri yapısı.
  */
struct InputState
{
    bool active = true;                              ///< Girdi okumanın aktif olup olmadığını belirtir.
    bool cursorLocked = false;                        ///< Fare imlecinin kilitli (disabled) olup olmadığı.

    bool keys[GLFW_KEY_LAST + 1]{};                  ///< Mevcut karedeki tuş basılma durumları.
    bool prevKeys[GLFW_KEY_LAST + 1]{};              ///< Bir önceki karedeki tuş durumları.

    bool actions[static_cast<int>(InputAction::COUNT)]{};     ///< Mevcut karedeki eylem durumları.
    bool prevActions[static_cast<int>(InputAction::COUNT)]{}; ///< Bir önceki karedeki eylem durumları.

    glm::vec2 mousePos{ 0.0f, 0.0f };                 ///< İmlecin ekran üzerindeki anlık konumu.
    glm::vec2 mouseDelta{ 0.0f, 0.0f };               ///< İmlecin kareler arası yer değiştirme miktarı.
    glm::vec2 gameMouseDelta{ 0.0f, 0.0f };           ///< Oyun penceresine göre ölçeklenmiş yer değiştirme.

    glm::vec2 gameViewportSize{ 1.0f, 1.0f };         ///< Oyun penceresi genişlik ve yüksekliği.
    glm::vec2 gameViewportPos{ 0.0f, 0.0f };          ///< Oyun penceresinin ekran konumu.

    bool firstMouse = true;                           ///< İlk fare hareketinde ani sıçramayı engelleyen bayrak.
};