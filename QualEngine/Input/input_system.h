#pragma once

struct GLFWwindow;
struct InputState;
class InputMap;

class InputSystem
{
public:
    void update(GLFWwindow* window, InputState& state, InputMap& map);

};