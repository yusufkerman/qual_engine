#include "input_map.h"

void InputMap::bind(int key, InputAction action)
{
    keyToAction[key] = action;
}

const std::unordered_map<int, InputAction>& InputMap::getBindings() const
{
    return keyToAction;
}