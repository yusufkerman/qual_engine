#pragma once

/**
 * @file input_utils.h
 * @brief Tuş ve eylem durumlarını sorgulamak için yardımcı satır içi (inline) fonksiyonlar.
 */

#include "input_state.h"

 /**
  * @brief Belirtilen tuşun o an basılı olup olmadığını sorgular.
  */
inline bool isKeyDown(const InputState& s, int key)
{
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return s.keys[key];
}

/**
 * @brief Belirtilen tuşa tam bu karede yeni basılıp basılmadığını sorgular (Down edge).
 */
inline bool isKeyPressed(const InputState& s, int key)
{
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return s.keys[key] && !s.prevKeys[key];
}

/**
 * @brief Belirtilen tuşun tam bu karede bırakılıp bırakılmadığını sorgular (Up edge).
 */
inline bool isKeyReleased(const InputState& s, int key)
{
    if (key < 0 || key > GLFW_KEY_LAST) return false;
    return !s.keys[key] && s.prevKeys[key];
}

/**
 * @brief Belirtilen eylemin o an aktif olup olmadığını sorgular.
 */
inline bool isActionDown(const InputState& s, InputAction action)
{
    auto idx = static_cast<int>(action);
    if (idx < 0 || idx >= static_cast<int>(InputAction::COUNT)) return false;
    return s.actions[idx];
}

/**
 * @brief Belirtilen eylemin tam bu karede başlayıp başlamadığını sorgular.
 */
inline bool isActionPressed(const InputState& s, InputAction action)
{
    auto idx = static_cast<int>(action);
    if (idx < 0 || idx >= static_cast<int>(InputAction::COUNT)) return false;
    return s.actions[idx] && !s.prevActions[idx];
}

/**
 * @brief Belirtilen eylemin tam bu karede sonlanıp sonlanmadığını sorgular.
 */
inline bool isActionReleased(const InputState& s, InputAction action)
{
    auto idx = static_cast<int>(action);
    if (idx < 0 || idx >= static_cast<int>(InputAction::COUNT)) return false;
    return !s.actions[idx] && s.prevActions[idx];
}