#include "editor_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

void EditorCamera::resetMouseState()
{
    capturingMouse = false;
    firstMouse = true;
    prevRightHeld = false;
    scrollStep = 0.0f;
}

void EditorCamera::update(
    GLFWwindow* window,
    InputState& input,
    float sensitivity,
    float moveSpeed
)
{
    bool rightHeld =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    bool shiftHeld =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    float speed = moveSpeed * (shiftHeld ? 3.0f : 1.0f);

    if (sceneHovered && rightHeld && !prevRightHeld)
    {
        capturingMouse = true;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (!rightHeld && prevRightHeld)
    {
        capturingMouse = false;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    prevRightHeld = rightHeld;

    if (!capturingMouse)
        return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = float(xpos - lastX);
    float yoffset = float(lastY - ypos);

    lastX = xpos;
    lastY = ypos;

    rotation.y += xoffset * sensitivity;
    rotation.x += yoffset * sensitivity;

    rotation.x = std::clamp(rotation.x, -1.5f, 1.5f);

    glm::vec3 forward;
    forward.x = cos(rotation.x) * sin(rotation.y);
    forward.y = sin(rotation.x);
    forward.z = -cos(rotation.x) * cos(rotation.y);
    forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(glm::cross(forward, { 0,1,0 }));

    if (input.actions[(int)InputAction::MoveForward])
        position += forward * speed;

    if (input.actions[(int)InputAction::MoveBackward])
        position -= forward * speed;

    if (input.actions[(int)InputAction::MoveRight])
        position += right * speed;

    if (input.actions[(int)InputAction::MoveLeft])
        position -= right * speed;

    position += forward * scrollStep * 1.5f;
    scrollStep = 0.0f;
}
glm::mat4 EditorCamera::getViewMatrix() const
{
    glm::vec3 forward;
    forward.x = cos(rotation.x) * sin(rotation.y);
    forward.y = sin(rotation.x);
    forward.z = -cos(rotation.x) * cos(rotation.y);

    forward = glm::normalize(forward);

    return glm::lookAt(position, position + forward, glm::vec3(0, 1, 0));
}

glm::mat4 EditorCamera::getProjectionMatrix(float aspect) const
{
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}