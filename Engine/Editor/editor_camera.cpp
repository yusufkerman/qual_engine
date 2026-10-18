/**
 * @file editor_camera.cpp
 * @brief Editör serbest uçuş kamerası hesaplamalarını ve girdi yönetimini içerir.
 */

#include "editor_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>

void EditorCamera::set2DMode(bool enabled)
{
    if (is2DMode == enabled)
        return;

    is2DMode = enabled;

    if (is2DMode)
    {
        saved3DPosition = position;
        saved3DRotation = rotation;

        // 2D mod: Kamerayı XY düzlemine dik, -Z yönüne bakacak şekilde ayarla
        rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        if (position.z <= 0.0f) {
            position.z = 20.0f;
        }
    }
    else
    {
        // 3D moduna geri dönüş
        position = saved3DPosition;
        rotation = saved3DRotation;
    }
}

void EditorCamera::toggle2DMode()
{
    set2DMode(!is2DMode);
}

void EditorCamera::resetMouseState()
{
    flyState = fly_camera_state{};
    scrollStep = 0.0f;
}

void EditorCamera::update(
    GLFWwindow* window,
    InputState& input,
    float sensitivity,
    float moveSpeed,
    bool hoverGate
)
{
    if (!window)
        return;

    if (is2DMode)
    {
        const bool rightHeld = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        const bool middleHeld = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
        const bool shiftHeld = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
        const float panSpeed = (shiftHeld ? 3.0f : 1.0f) * (orthographicSize / 600.0f);

        if ((rightHeld || middleHeld) && !flyState.prevRightHeld && hoverGate)
        {
            flyState.capturingMouse = true;
            flyState.firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        if (!(rightHeld || middleHeld) && flyState.prevRightHeld)
        {
            flyState.capturingMouse = false;
            flyState.firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        flyState.prevRightHeld = (rightHeld || middleHeld);

        if (flyState.capturingMouse)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            if (flyState.firstMouse)
            {
                flyState.lastX = xpos;
                flyState.lastY = ypos;
                flyState.firstMouse = false;
            }

            const float xoffset = static_cast<float>(xpos - flyState.lastX);
            const float yoffset = static_cast<float>(flyState.lastY - ypos);

            flyState.lastX = xpos;
            flyState.lastY = ypos;

            position.x -= xoffset * panSpeed;
            position.y -= yoffset * panSpeed;
        }

        // Fare tekerleği ile Ortografik Yakınlaştırma (Zoom)
        if (scrollStep != 0.0f && hoverGate)
        {
            orthographicSize = std::clamp(orthographicSize - scrollStep * (orthographicSize * 0.15f), 0.1f, 10000.0f);
            scrollStep = 0.0f;
        }

        // Klavye ile Pan (W/S/A/D)
        if (hoverGate || flyState.capturingMouse)
        {
            const float keySpeed = moveSpeed * (shiftHeld ? 3.0f : 1.0f) * (orthographicSize / 40.0f) * 0.016f;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position.y += keySpeed;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position.y -= keySpeed;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position.x -= keySpeed;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position.x += keySpeed;
        }

        // 2D modda rotasyon her zaman XY düzlemine dik sabit kalır
        rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        return;
    }

    update_fly_camera(
        window,
        input,
        flyState,
        position,
        rotation,
        sensitivity,
        moveSpeed,
        hoverGate,
        &scrollStep
    );
}

glm::mat4 EditorCamera::getViewMatrix() const
{
    const glm::vec3 forward = calc_forward(rotation);
    return glm::lookAt(position, position + forward, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 EditorCamera::getProjectionMatrix(float aspect) const
{
    if (is2DMode)
    {
        const float safeAspect = (aspect > 0.01f) ? aspect : (16.0f / 9.0f);
        const float halfH = orthographicSize * 0.5f;
        const float halfW = halfH * safeAspect;
        return glm::ortho(-halfW, halfW, -halfH, halfH, nearPlane, farPlane);
    }

    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}