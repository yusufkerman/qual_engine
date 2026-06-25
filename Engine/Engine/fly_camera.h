#pragma once

#include <glm/glm.hpp>

struct GLFWwindow;
struct InputState;

struct fly_camera_state
{
    bool capturingMouse = false;
    bool firstMouse = true;
    bool prevRightHeld = false;
    double lastX = 0.0;
    double lastY = 0.0;
};

glm::vec3 calc_forward(const glm::vec3& rotation);

void update_fly_camera(
    GLFWwindow* window,
    InputState& input,
    fly_camera_state& state,
    glm::vec3& position,
    glm::vec3& rotation,
    float sensitivity,
    float moveSpeed,
    bool hoverGate,
    float* scrollStep = nullptr
);