#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>

class ui_manager
{
public:
    void init(GLFWwindow* window);
    void beginFrame();
    void endFrame();
    void shutdown();

private:
    GLFWwindow* window = nullptr;

    void setupDockLayout(ImGuiID dockspace_id, const ImVec2& size);
};