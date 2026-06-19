#include "editor_camera.h"

#include <glm/gtc/matrix_transform.hpp>

void EditorCamera::resetMouseState()
{
    flyState = fly_camera_state{};
    scrollStep = 0.0f;
}

void EditorCamera::update(
    GLFWwindow* window,
    InputState& input,
    float sensitivity,
    float moveSpeed
)
{
    UpdateFlyCamera(
        window,
        input,
        flyState,
        position,
        rotation,
        sensitivity,
        moveSpeed,
        sceneHovered
    );

    if (!flyState.capturingMouse)
        return;

    const glm::vec3 forward = CalcForward(rotation);
    position += forward * scrollStep * 1.5f;
    scrollStep = 0.0f;
}

glm::mat4 EditorCamera::getViewMatrix() const
{
    const glm::vec3 forward = CalcForward(rotation);
    return glm::lookAt(position, position + forward, glm::vec3(0, 1, 0));
}

glm::mat4 EditorCamera::getProjectionMatrix(float aspect) const
{
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}
