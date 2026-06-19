#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../Input/input_state.h"

class EditorCamera
{
public:
    bool sceneFocused = false;
    bool sceneHovered = false;

    bool capturingMouse = false;
    bool firstMouse = true;

    bool prevRightHeld = false;

    double lastX = 0.0;
    double lastY = 0.0;

    float scrollStep = 0.0f;

    glm::vec3 position = glm::vec3(0.0f, -5.0f, 70.0f);
    glm::vec3 rotation = glm::vec3(0.0f);

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    void update(
        GLFWwindow* window,
        InputState& input,
        float sensitivity,
        float moveSpeed
    );

    void resetMouseState();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
};