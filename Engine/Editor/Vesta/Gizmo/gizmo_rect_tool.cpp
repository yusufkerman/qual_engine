/**
 * @file gizmo_rect_tool.cpp
 * @brief 2B Oyun İçi UI dikdörtgen boyutlandırma ve taşıma aracı uygulaması.
 */

#include "gizmo_rect_tool.h"
#include "../../../Engine/engine_context.h"
#include "../../../ECS/components.h"
#include <algorithm>
#include <cmath>

namespace vesta
{
    void gizmo_rect_tool::reset_interaction()
    {
        m_Active = Handle::None;
        m_Hovered = Handle::None;
        m_DragEntity = 0;
    }

    void gizmo_rect_tool::render(
        EngineContext& engineCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        scene& activeScene = engineCtx.session.editor_scene();
        registry& reg = activeScene.get_registry();
        const auto& selectedEntities = engineCtx.vestaSys.selection().get_selected_entities();
        if (selectedEntities.empty())
        {
            reset_interaction();
            return;
        }

        entity selectedEnt = *selectedEntities.begin();
        if (selectedEnt == 0 || !reg.has_component<rect_transform_component>(selectedEnt))
        {
            reset_interaction();
            return;
        }

        auto* rt = reg.get_component<rect_transform_component>(selectedEnt);
        if (!rt) return;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 mousePos = ImGui::GetIO().MousePos;
        const bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

        // rt->computed_screen_rect önbelleğinden doğru ekran sınırlarını al
        float minX = rt->computed_screen_rect.x;
        float minY = rt->computed_screen_rect.y;
        float maxX = minX + rt->computed_screen_rect.z;
        float maxY = minY + rt->computed_screen_rect.w;

        if (rt->computed_screen_rect.z <= 0.0f || rt->computed_screen_rect.w <= 0.0f)
        {
            const glm::vec2 halfSize = (rt->size * rt->scale) * 0.5f;
            minX = viewportPos.x + viewportSize.x * 0.5f + rt->anchored_position.x - halfSize.x;
            maxX = minX + rt->size.x * rt->scale.x;
            minY = viewportPos.y + viewportSize.y * 0.5f - rt->anchored_position.y - halfSize.y;
            maxY = minY + rt->size.y * rt->scale.y;
        }

        const ImU32 boxOutlineCol = IM_COL32(65, 160, 255, 230);
        const ImU32 boxFillCol = IM_COL32(65, 160, 255, 25);
        const ImU32 handleFillCol = IM_COL32(245, 245, 250, 240);
        const ImU32 handleHoverCol = IM_COL32(255, 220, 50, 255);
        const ImU32 handleOutlineCol = IM_COL32(20, 22, 28, 220);

        // 1. Ana Dikdörtgen Çizimi
        dl->AddRectFilled(ImVec2(minX, minY), ImVec2(maxX, maxY), boxFillCol);
        dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), boxOutlineCol, 0.0f, 0, 1.5f);

        // Tutamaç noktaları
        const float hRadius = 5.0f;
        auto drawHandle = [&](Handle id, const ImVec2& pos, ImGuiMouseCursor cursor) -> bool
        {
            const bool hovered = (std::abs(mousePos.x - pos.x) <= hRadius + 2.0f && std::abs(mousePos.y - pos.y) <= hRadius + 2.0f);
            if (hovered && m_Active == Handle::None)
            {
                m_Hovered = id;
                ImGui::SetMouseCursor(cursor);
            }

            const bool hot = (m_Hovered == id || m_Active == id);
            dl->AddRectFilled(ImVec2(pos.x - hRadius, pos.y - hRadius), ImVec2(pos.x + hRadius, pos.y + hRadius), hot ? handleHoverCol : handleFillCol, 1.0f);
            dl->AddRect(ImVec2(pos.x - hRadius, pos.y - hRadius), ImVec2(pos.x + hRadius, pos.y + hRadius), handleOutlineCol, 1.0f, 0, 1.2f);
            return hot;
        };

        if (m_Active == Handle::None)
        {
            m_Hovered = Handle::None;
        }

        // 8 Tutamaç: 4 Köşe + 4 Kenar
        drawHandle(Handle::TopLeft, ImVec2(minX, minY), ImGuiMouseCursor_ResizeNWSE);
        drawHandle(Handle::TopRight, ImVec2(maxX, minY), ImGuiMouseCursor_ResizeNESW);
        drawHandle(Handle::BottomLeft, ImVec2(minX, maxY), ImGuiMouseCursor_ResizeNESW);
        drawHandle(Handle::BottomRight, ImVec2(maxX, maxY), ImGuiMouseCursor_ResizeNWSE);

        drawHandle(Handle::Left, ImVec2(minX, (minY + maxY) * 0.5f), ImGuiMouseCursor_ResizeEW);
        drawHandle(Handle::Right, ImVec2(maxX, (minY + maxY) * 0.5f), ImGuiMouseCursor_ResizeEW);
        drawHandle(Handle::Top, ImVec2((minX + maxX) * 0.5f, minY), ImGuiMouseCursor_ResizeNS);
        drawHandle(Handle::Bottom, ImVec2((minX + maxX) * 0.5f, maxY), ImGuiMouseCursor_ResizeNS);

        // Kutu içi gövde hover (Taşıma)
        if (m_Hovered == Handle::None)
        {
            if (mousePos.x >= minX && mousePos.x <= maxX && mousePos.y >= minY && mousePos.y <= maxY)
            {
                m_Hovered = Handle::Body;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
        }

        // 2. Etkileşim ve Sürükleme Mantığı
        if (m_Active == Handle::None)
        {
            if (isMouseDown && m_Hovered != Handle::None)
            {
                m_Active = m_Hovered;
                m_DragEntity = selectedEnt;
                m_DragStartPosition = glm::vec3(rt->anchored_position.x, rt->anchored_position.y, 0.0f);
                m_DragStartScale = glm::vec3(rt->size.x, rt->size.y, 1.0f);
                m_DragStartMousePos = glm::vec2(mousePos.x, mousePos.y);
            }
        }
        else if (m_Active >= Handle::Body && m_Active <= Handle::Bottom)
        {
            if (!isMouseDown)
            {
                reset_interaction();
            }
            else
            {
                const glm::vec2 mouseDelta(mousePos.x - m_DragStartMousePos.x, mousePos.y - m_DragStartMousePos.y);

                if (m_Active == Handle::Body)
                {
                    rt->anchored_position.x = m_DragStartPosition.x + mouseDelta.x;
                    rt->anchored_position.y = m_DragStartPosition.y - mouseDelta.y;
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                }
                else if (m_Active == Handle::Right)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x + mouseDelta.x * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                }
                else if (m_Active == Handle::Left)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x - mouseDelta.x * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                }
                else if (m_Active == Handle::Bottom)
                {
                    rt->size.y = std::max(10.0f, m_DragStartScale.y + mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                else if (m_Active == Handle::Top)
                {
                    rt->size.y = std::max(10.0f, m_DragStartScale.y - mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                else if (m_Active == Handle::BottomRight)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x + mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y + mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                }
                else if (m_Active == Handle::TopLeft)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x - mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y - mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                }
                else if (m_Active == Handle::TopRight)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x + mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y - mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
                }
                else if (m_Active == Handle::BottomLeft)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x - mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y + mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
                }
            }
        }
    }
} // namespace vesta

