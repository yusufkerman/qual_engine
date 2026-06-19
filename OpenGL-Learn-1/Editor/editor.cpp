#include "editor.h"
#include "../Scene/scene.h"

#include <imgui.h>
#include <string>

void Editor::init(GLFWwindow*) {}

void Editor::update(scene&)
{
    // şimdilik boş
}

void Editor::renderUI(scene& scene)
{
    auto& reg = scene.get_registry();

    // =========================
    // TOP BAR
    // =========================
    ImGui::Begin("TopBar");

    if (ImGui::Button(mode == EngineMode::EDITOR ? "Play" : "Stop"))
    {
        toggleMode();
    }

    ImGui::SameLine();
    ImGui::Text("Mode: %s", mode == EngineMode::EDITOR ? "EDITOR" : "PLAY");

    ImGui::End();

    // =========================
    // HIERARCHY
    // =========================
    ImGui::Begin("Hierarchy");

    for (auto e : reg.get_all_entities())
    {
        std::string label = "Entity " + std::to_string(e);
        bool selected = (selectedEntity == (int)e);

        if (ImGui::Selectable(label.c_str(), selected))
            selectedEntity = (int)e;
    }

    ImGui::End();

    // =========================
    // SCENE VIEWPORT
    // =========================
    ImGui::Begin("Scene");

    sceneViewportHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    editorCamera.sceneFocused =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    editorCamera.sceneHovered = sceneViewportHovered;

    ImGui::PushID("SceneViewport");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImVec2 avail = ImGui::GetContentRegionAvail();

    if (avail.x > 1.0f && avail.y > 1.0f)
        setSceneViewportSize({ avail.x, avail.y });

    if (avail.x > 0 && avail.y > 0 && sceneTexture != 0)
    {
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        ImGui::GetWindowDrawList()->PushClipRect(
            cursor,
            ImVec2(cursor.x + avail.x, cursor.y + avail.y),
            true
        );

        ImGui::Image(
            (ImTextureID)(intptr_t)sceneTexture,
            avail,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        ImGui::GetWindowDrawList()->PopClipRect();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopID();

    ImGui::End();

    // =========================
    // GAME VIEWPORT
    // =========================
    ImGui::Begin("Game");

    gameViewportHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    ImGui::PushID("GameViewport");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImVec2 gameAvail = ImGui::GetContentRegionAvail();

    if (gameAvail.x > 1.0f && gameAvail.y > 1.0f)
        setGameViewportSize({ gameAvail.x, gameAvail.y });

    if (gameAvail.x > 0 && gameAvail.y > 0 && gameTexture != 0)
    {
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        ImGui::GetWindowDrawList()->PushClipRect(
            cursor,
            ImVec2(cursor.x + gameAvail.x, cursor.y + gameAvail.y),
            true
        );

        ImGui::Image(
            (ImTextureID)(intptr_t)gameTexture,
            gameAvail,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        ImGui::GetWindowDrawList()->PopClipRect();
    }

    ImGui::PopStyleVar(2);
    ImGui::PopID();

    ImGui::End();

    // =========================
    // INSPECTOR
    // =========================
    ImGui::Begin("Inspector");

    if (selectedEntity != -1)
    {
        entity e = (entity)selectedEntity;

        ImGui::Text("Entity %d", selectedEntity);

        if (reg.has_transform(e))
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                if (ImGui::SmallButton("X##transform"))
                    reg.remove_transform(e);

                auto* t = reg.get_transform(e);
                ImGui::DragFloat3("Position", &t->position.x, 0.1f);
                ImGui::DragFloat3("Rotation", &t->rotation.x, 0.1f);
                ImGui::DragFloat3("Scale", &t->scale.x, 0.1f);
            }
        }

        if (reg.has_camera(e))
        {
            if (ImGui::CollapsingHeader("Camera"))
            {
                if (ImGui::SmallButton("X##camera"))
                {
                    if (scene.get_active_camera() == e)
                        scene.set_active_camera(0);

                    reg.remove_camera(e);
                }

                auto* c = reg.get_camera(e);

                if (ImGui::Button("Set as Game Camera"))
                {
                    scene.set_active_camera(e);
                }

                ImGui::DragFloat("FOV", &c->fov);
                ImGui::DragFloat("Near", &c->nearPlane);
                ImGui::DragFloat("Far", &c->farPlane);
            }
        }

        if (reg.has_light(e))
        {
            if (ImGui::CollapsingHeader("Light"))
            {
                if (ImGui::SmallButton("X##light"))
                    reg.remove_light(e);

                auto* l = reg.get_light(e);
                ImGui::ColorEdit3("Color", &l->color.x);
                ImGui::DragFloat("Intensity", &l->intensity);
            }
        }

        if (reg.has_mesh(e))
        {
            if (ImGui::CollapsingHeader("Mesh"))
            {
                if (ImGui::SmallButton("X##mesh"))
                    reg.remove_mesh(e);
            }
        }

        ImGui::Separator();

        if (ImGui::BeginMenu("Add Component"))
        {
            if (ImGui::MenuItem("Transform"))
                if (!reg.has_transform(e))
                    reg.add_transform(e, {});

            if (ImGui::MenuItem("Camera"))
                if (!reg.has_camera(e))
                    reg.add_camera(e, { 45.0f, 0.1f, 1000.0f });

            if (ImGui::MenuItem("Light"))
                if (!reg.has_light(e))
                    reg.add_light(e, {});

            if (ImGui::MenuItem("Mesh"))
                if (!reg.has_mesh(e))
                    reg.add_mesh(e, { 0 });

            ImGui::EndMenu();
        }
    }

    ImGui::End();
}