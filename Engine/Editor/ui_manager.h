#pragma once

/**
 * @file ui_manager.h
 * @brief ImGui bağlamını, platform (GLFW/OpenGL3) arka uçlarını ve ana yerleşim (DockSpace) düzenini yöneten sınıf.
 */

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <functional>

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
     * @param mainMenuBarCallback İsteğe bağlı ana menü çubuğu (File, Window vb.) çizim fonksiyonu.
     */
    void beginFrame(std::function<void()> mainMenuBarCallback = nullptr);

    /**
     * @brief Çizim verilerini derler ve OpenGL üzerinden ekrana yansıtır.
     */
    void endFrame();

    /**
     * @brief ImGui arka uçlarını ve bağlamını güvenle kapatır.
     */
    void shutdown();

    /**
     * @brief DockSpace panel yerleşimini bir sonraki karede varsayılan düzene sıfırlar.
     */
    void resetLayout() { m_resetLayout = true; }

private:
    GLFWwindow* m_window = nullptr;
    bool m_resetLayout = false;

    /**
     * @brief İlk açılışta varsayılan panel yerleşimini (TopBar, Inspector, Hierarchy, Viewports) inşa eder.
     */
    void setupDockLayout(ImGuiID dockspace_id, const ImVec2& size);
};