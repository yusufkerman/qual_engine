#pragma once

/**
 * @file input_action.h
 * @brief Donanım tuşlarından soyutlanmış mantıksal oyun eylemlerini tanımlar.
 */

 /**
  * @enum InputAction
  * @brief Tuş eşlemelerinde kullanılan mantıksal eylem tanımları.
  */
enum class InputAction
{
    MoveForward,  ///< İleri hareket eylemi.
    MoveBackward, ///< Geri hareket eylemi.
    MoveLeft,     ///< Sola hareket eylemi.
    MoveRight,    ///< Sağa hareket eylemi.
    Jump,         ///< Zıplama eylemi.
    COUNT         ///< Eylem sayısı (dizi boyutlandırması için).
};