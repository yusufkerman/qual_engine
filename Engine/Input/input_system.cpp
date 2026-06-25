#include "input_system.h"
#include "input_state.h"
#include "input_map.h"

#include <GLFW/glfw3.h>

void InputSystem::update(GLFWwindow* window,
    InputState& state,
    InputMap& map)
{
    for (int i = 0; i <= GLFW_KEY_LAST; i++)
        state.prevKeys[i] = state.keys[i];

    for (int i = 0; i <= GLFW_KEY_LAST; i++)
        state.keys[i] = glfwGetKey(window, i) == GLFW_PRESS;

    const auto& bindings = map.getBindings();

    for (auto& [key, action] : bindings)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            continue;

        int a = static_cast<int>(action);

        state.prevActions[a] = state.actions[a];
        state.actions[a] = state.keys[key];
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (state.firstMouse)
    {
        state.mousePos = { x, y };
        state.firstMouse = false;
    }

    state.mouseDelta = {
        x - state.mousePos.x,
        y - state.mousePos.y
    };

    state.mousePos = { x, y };
}