/**
 * @file input_map.cpp
 * @brief Tuş ve eylem eşleme tablosunun uygulaması.
 */

#include "input_map.h"

void InputMap::bind(int key, InputAction action)
{
    m_keyToAction[key] = action;
}

const std::unordered_map<int, InputAction>& InputMap::getBindings() const
{
    return m_keyToAction;
}