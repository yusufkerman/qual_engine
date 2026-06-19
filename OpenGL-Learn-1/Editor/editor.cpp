#include "editor.h"
#include "../Scene/scene.h"

#include <imgui.h>
#include <string>
#include <imgui_internal.h>

void Editor::init(GLFWwindow*) {}

void Editor::update(scene&) {}

// =========================
// SAFE NAME GET
// =========================
static std::string getEntityName(auto& reg, entity e)
{
    if (reg.has_name(e))
        return reg.get_name(e)->name;

    return "Entity";
}

void Editor::renderUI(scene& scene)
{
    auto& reg = scene.get_registry();

    // =========================
    // TOP BAR
    // =========================
    ImGui::Begin("TopBar");

    if (ImGui::Button(mode == EngineMode::EDITOR ? "Play" : "Stop"))
        toggleMode();

    ImGui::SameLine();
    ImGui::Text("Mode: %s", mode == EngineMode::EDITOR ? "EDITOR" : "PLAY");

    ImGui::End();

    // =========================
    // HIERARCHY
    // =========================
    ImGui::Begin("Hierarchy");

    // CREATE OBJECT (UNITY STYLE)
    if (ImGui::Button("Create Object"))
    {
        entity e = reg.create_entity();

        reg.add_transform(e, {});
        reg.add_name(e, { "New Entity" });

        selectedEntity = (int)e;

        // rename immediately
        renamingEntity = (int)e;
        strcpy_s(renameBuffer, sizeof(renameBuffer), "New Entity");
    }

    ImGui::Separator();

    // =========================
    // ENTITY LIST
    // =========================
    for (auto e : reg.get_all_entities())
    {
        std::string name = getEntityName(reg, e);
        bool isSelected = (selectedEntity == (int)e);

        ImGui::PushID((int)e);

        if (renamingEntity != (int)e)
        {
            if (ImGui::Selectable(name.c_str(), isSelected))
                selectedEntity = (int)e;

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    renamingEntity = (int)e;
                    strcpy_s(renameBuffer, sizeof(renameBuffer), name.c_str());
                }

                if (ImGui::MenuItem("Delete"))
                {
                    if (selectedEntity == (int)e)
                        selectedEntity = -1;

                    deleteQueue.push_back(e);  
                }

                ImGui::EndPopup();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                renamingEntity = (int)e;
                strcpy_s(renameBuffer, sizeof(renameBuffer), name.c_str());
            }
        }
        else
        {
            ImGui::PushID((int)e);

            ImGui::SetNextItemWidth(160);

            bool enter = ImGui::InputText(
                "##rename",
                renameBuffer,
                IM_ARRAYSIZE(renameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue
            );

            ImGui::SameLine();

            bool cancel = ImGui::SmallButton("X");

            bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape);

            if (enter || cancel || esc)
            {
                if (enter)
                {
                    if (auto* n = reg.get_name(e))
                        n->name = renameBuffer;
                    else
                        reg.add_name(e, { renameBuffer });
                }

                renamingEntity = -1;
            }

            ImGui::PopID();
        }

        ImGui::PopID();
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
        ImGui::Image(
            (ImTextureID)(intptr_t)gameTexture,
            gameAvail,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );
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
                auto* c = reg.get_camera(e);

                ImGui::DragFloat("FOV", &c->fov);
                ImGui::DragFloat("Near", &c->nearPlane);
                ImGui::DragFloat("Far", &c->farPlane);
            }
        }

        if (reg.has_light(e))
        {
            if (ImGui::CollapsingHeader("Light"))
            {
                auto* l = reg.get_light(e);

                ImGui::ColorEdit3("Color", &l->color.x);
                ImGui::DragFloat("Intensity", &l->intensity);
            }
        }
    }

    ImGui::End();

    for (entity e : deleteQueue)
    {
        if (selectedEntity == (int)e)
            selectedEntity = -1;

        if (renamingEntity == (int)e)
            renamingEntity = -1;

        reg.remove_entity(e);
    }
    deleteQueue.clear();
}