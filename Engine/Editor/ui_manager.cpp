#include "ui_manager.h"

#include "ComponentUI/transform_component_ui.h"
#include "ComponentUI/camera_component_ui.h"
#include "ComponentUI/light_component_ui.h"
#include "ComponentUI/name_component_ui.h"
#include "ComponentUI/mesh_component_ui.h"
#include "ComponentUI/material_component_ui.h"
#include "ComponentUI/rigidbody_component_ui.h"
#include "ComponentUI/collider_component_ui.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui_internal.h>



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

    // Top bar
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Up, 0.08f, &top, &center);

    // Inspector
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.22f, &left, &center);

    // Hierarchy
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.22f, &right, &center);

    // Game view
    ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.30f, &bottom, &center);

    ImGui::DockBuilderDockWindow("TopBar", top);
    ImGui::DockBuilderDockWindow("Inspector", left);
    ImGui::DockBuilderDockWindow("Hierarchy", right);
    ImGui::DockBuilderDockWindow("Scene", center);
    ImGui::DockBuilderDockWindow("Game", bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}

void ui_manager::init(GLFWwindow* window_)
{
    window = window_;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    //Yeni sisteme kayıtların yapıldığı yer.
    register_transform_ui_system();
    register_camera_ui_system();
    register_light_ui_system();
    register_name_ui_system();
    register_mesh_ui_system();
    register_material_ui_system();
    register_rigidbody_ui_system();
    register_collider_ui_system();
}

void ui_manager::beginFrame()
{
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiID dockspace_id = ImGui::GetID("MainDock");

    static bool first = true;
    if (first)
    {
        first = false;
        setupDockLayout(dockspace_id, io.DisplaySize);
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

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
}