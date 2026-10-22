#pragma once

#include <glm/glm.hpp>

#include "../Engine/fly_camera.h"

struct GLFWwindow;
struct InputState;

class EditorCamera
{
public:
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };

    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float scrollStep = 0.0f;

    bool sceneHovered = false;
    bool sceneFocused = false;

    fly_camera_state flyState;

    void resetMouseState();

    void update(
        GLFWwindow* window,
        InputState& input,
        float sensitivity,
        float moveSpeed
    );

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
};