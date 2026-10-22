#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "input_action.h"

struct InputState
{
    bool keys[GLFW_KEY_LAST + 1]{};
    bool prevKeys[GLFW_KEY_LAST + 1]{};

    bool actions[(int)InputAction::COUNT]{};
    bool prevActions[(int)InputAction::COUNT]{};

    glm::vec2 mousePos{ 0.0f, 0.0f };
    glm::vec2 mouseDelta{ 0.0f, 0.0f };
    bool firstMouse = true;
};