#pragma once

/**
 * @file input_map.h
 * @brief Fiziksel tuş kodları ile mantıksal eylemler arasındaki eşlemeleri yöneten sınıf.
 */

#include <unordered_map>
#include <GLFW/glfw3.h>
#include "input_action.h"

 /**
  * @class InputMap
  * @brief GLFW tuş kodlarını InputAction eylemlerine bağlayan eşleme tablosu.
  */
class InputMap
{
public:
    InputMap() = default;
    ~InputMap() = default;

    /**
     * @brief Fiziksel bir tuşu mantıksal bir eyleme bağlar.
     * @param key GLFW tuş kodu (örn. GLFW_KEY_W).
     * @param action Tetiklenecek mantıksal eylem.
     */
    void bind(int key, InputAction action);

    /**
     * @brief Mevcut tuş eşleme tablosunu salt okunur referans olarak döndürür.
     */
    const std::unordered_map<int, InputAction>& getBindings() const;

private:
    std::unordered_map<int, InputAction> m_keyToAction;
};