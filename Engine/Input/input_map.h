#pragma once
#include <unordered_map>
#include <GLFW/glfw3.h>

#include "input_action.h"

class InputMap
{
public:
    void bind(int key, InputAction action);

    const std::unordered_map<int, InputAction>& getBindings() const;

private:
    std::unordered_map<int, InputAction> keyToAction;
};