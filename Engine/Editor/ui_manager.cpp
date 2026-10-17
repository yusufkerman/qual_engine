/**
 * @file ui_manager.cpp
 * @brief ImGui arka uç entegrasyonu ve DockSpace düğüm bölme işlemlerini içerir.
 */

#include "ui_manager.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <filesystem>

void ui_manager::setupDockLayout(ImGuiID dockspace_id, const ImVec2& size)
{
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(
        dockspace_id,
        ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode
    );

    ImGui::DockBuilderSetNodeSize(dockspace_id, size);

    ImGuiID top = 0;
    ImGuiID left = 0;
    ImGuiID right = 0;
    ImGuiID center = 0;
    ImGuiID bottom = 0;

    center = dockspace_id;

    // Üst kontrol çubuğu
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Up, 0.08f, &top, &center);

    // Inspector (Sol panel)
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.22f, &left, &center);

    // Hierarchy ve Asset Browser (Sağ panel - Üst: Hierarchy, Alt: Asset Browser)
    ImGuiID right_bottom = 0;
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.24f, &right, &center);
    ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.45f, &right_bottom, &right);

    // Game Viewport (Alt panel)
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.30f, &bottom, &center);

    // Panelleri kimliklerine göre yuvalarına sabitle
    ImGui::DockBuilderDockWindow("TopBar", top);
    ImGui::DockBuilderDockWindow("Inspector", left);
    ImGui::DockBuilderDockWindow("Lighting", left);
    ImGui::DockBuilderDockWindow("Rendering Settings", left);
    ImGui::DockBuilderDockWindow("Hierarchy", right);
    ImGui::DockBuilderDockWindow("Asset Browser", right_bottom);
    ImGui::DockBuilderDockWindow("Scene", center);
    ImGui::DockBuilderDockWindow("Game", bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}

void ui_manager::init(GLFWwindow* window)
{
    m_window = window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    // Türkçe karakter desteği için genişletilmiş karakter aralıkları
    static const ImWchar turkish_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement (ç, Ç, ö, Ö, ü, Ü vs.)
        0x0100, 0x017F, // Latin Extended-A (ğ, Ğ, ı, İ, ş, Ş vs.)
        0x2000, 0x206F, // General Punctuation
        0x20A0, 0x20CF, // Currency Symbols (₺ vs.)
        0,
    };

    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 1;
    font_cfg.PixelSnapH = true;

    // Windows sistem fontlarını sırayla dene: Segoe UI -> Arial -> Tahoma
    const char* font_paths[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf"
    };

    bool font_loaded = false;
    for (const char* path : font_paths) {
        if (std::filesystem::exists(path)) {
            io.Fonts->AddFontFromFileTTF(path, 16.0f, &font_cfg, turkish_ranges);
            font_loaded = true;
            break;
        }
    }

    if (!font_loaded) {
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void ui_manager::beginFrame(std::function<void()> mainMenuBarCallback)
{
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(m_window, &w, &h);
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    float menuBarHeight = 0.0f;
    if (mainMenuBarCallback)
    {
        if (ImGui::BeginMainMenuBar())
        {
            mainMenuBarCallback();
            menuBarHeight = ImGui::GetWindowSize().y;
            ImGui::EndMainMenuBar();
        }
    }

    ImGuiID dockspace_id = ImGui::GetID("MainDock");

    static bool first_launch = true;
    if (first_launch || m_resetLayout)
    {
        first_launch = false;
        m_resetLayout = false;
        setupDockLayout(dockspace_id, io.DisplaySize);
    }

    // Ana arka plan penceresini tam ekran olarak sabitle (ana menü çubuğunun hemen altından başlar)
    ImGui::SetNextWindowPos(ImVec2(0.0f, menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - menuBarHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::Begin("DockRoot", nullptr, flags);
    ImGui::DockSpace(
        dockspace_id,
        ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_PassthruCentralNode
    );
    ImGui::End();
}

void ui_manager::endFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ui_manager::shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_window = nullptr;
}