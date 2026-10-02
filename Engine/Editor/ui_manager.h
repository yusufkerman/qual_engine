#pragma once

/**
 * @file ui_manager.h
 * @brief ImGui bağlamını, platform (GLFW/OpenGL3) arka uçlarını ve ana yerleşim (DockSpace) düzenini yöneten sınıf.
 */

#include <GLFW/glfw3.h>
#include <imgui.h>

 /**
  * @class ui_manager
  * @brief Editör arayüzünün başlatılmasını, çerçeve (frame) sınırlarını ve pencere kenetlenme (docking) düzenini sağlar.
  */
class ui_manager
{
public:
    ui_manager() = default;
    ~ui_manager() = default;

    /**
     * @brief ImGui bağlamını ve GLFW/OpenGL3 arka uçlarını başlatır.
     * @param window GLFW ana pencere tanıtıcısı.
     */
    void init(GLFWwindow* window);

    /**
     * @brief Yeni bir ImGui çerçevesi başlatır ve ana DockSpace'i oluşturur.
     */
    void beginFrame();

    /**
     * @brief Çizim verilerini derler ve OpenGL üzerinden ekrana yansıtır.
     */
    void endFrame();

    /**
     * @brief ImGui arka uçlarını ve bağlamını güvenle kapatır.
     */
    void shutdown();

private:
    GLFWwindow* m_window = nullptr;

    /**
     * @brief İlk açılışta varsayılan panel yerleşimini (TopBar, Inspector, Hierarchy, Viewports) inşa eder.
     */
    void setupDockLayout(ImGuiID dockspace_id, const ImVec2& size);
};