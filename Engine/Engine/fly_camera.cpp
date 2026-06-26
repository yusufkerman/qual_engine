#include "fly_camera.h"

#include "../Input/input_state.h"
#include "../Input/input_map.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

glm::vec3 calc_forward(const glm::vec3& rot)
{
    glm::vec3 forward;
    forward.x = std::cos(rot.x) * std::sin(rot.y);
    forward.y = std::sin(rot.x);
    forward.z = -std::cos(rot.x) * std::cos(rot.y);
    return glm::normalize(forward);
}

void update_fly_camera(
    GLFWwindow* window,
    InputState& input,
    fly_camera_state& state,
    glm::vec3& position,
    glm::vec3& rotation,
    float sensitivity,
    float moveSpeed,
    bool hoverGate,
    float* scrollStep
)
{
    if (!window)
        return;

    const bool rightHeld =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    const bool shiftHeld =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    const float speed = moveSpeed * (shiftHeld ? 3.0f : 1.0f);

    if (rightHeld && !state.prevRightHeld && hoverGate)
    {
        state.capturingMouse = true;
        state.firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (!rightHeld && state.prevRightHeld)
    {
        state.capturingMouse = false;
        state.firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    state.prevRightHeld = rightHeld;

    if (!state.capturingMouse)
        return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (state.firstMouse)
    {
        state.lastX = xpos;
        state.lastY = ypos;
        state.firstMouse = false;
    }

    const float xoffset = static_cast<float>(xpos - state.lastX);
    const float yoffset = static_cast<float>(state.lastY - ypos);

    state.lastX = xpos;
    state.lastY = ypos;

    rotation.y += xoffset * sensitivity;
    rotation.x += yoffset * sensitivity;

    rotation.x = std::clamp(rotation.x, -1.5f, 1.5f);

    const glm::vec3 forward = calc_forward(rotation);
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += forward * speed;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= forward * speed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += right * speed;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= right * speed;

    if (scrollStep)
    {
        position += forward * (*scrollStep) * 1.5f;
        *scrollStep = 0.0f;
    }
}