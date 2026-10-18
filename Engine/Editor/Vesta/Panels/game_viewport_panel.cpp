/**
 * @file game_viewport_panel.cpp
 * @brief Oyun görünümü (Game Viewport) panelinin ImGui ile çizim işlemlerini, çözünürlük/en-boy oranı seçimini ve UI renderını barındırır.
 */

#include "game_viewport_panel.h"
#include "../../../Gameplay/gameplay_input.h"
#include "../../../Engine/engine_context.h"
#include "../../../ECS/components.h"
#include <imgui.h>
#include <algorithm>

namespace vesta {

    game_viewport_panel::game_viewport_panel(
        game_viewport_view_model& vm,
        editor_viewport_state& viewportState,
        EngineContext* engineCtx)
        : m_vm(vm)
        , m_viewport_state(viewportState)
        , m_engine_ctx(engineCtx)
    {
    }

    void game_viewport_panel::render()
    {
        ImGui::Begin(m_title.c_str(), &m_is_open);

        // Çekirdek editör durumundan oyun ekranı verisine ulaş
        auto& state = m_viewport_state.game();

        // Girdi (Input) ve kısayolların doğru çalışması için odak/imleç durumlarını güncelle
        state.focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        state.hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        ImGui::PushID("GameViewport");

        // ========================================================
        // 1. ÜST KONTROL ÇUBUĞU (TOOLBAR - RESOLUTION SELECTOR)
        // ========================================================
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
        ImGui::BeginChild("GameToolbar", ImVec2(0, 30.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const char* aspectNames[] = {
            "Free Aspect",
            "16:9 (1920x1080)",
            "16:9 (1280x720)",
            "16:10 (1920x1200)",
            "4:3 (1024x768)",
            "21:9 (2560x1080)"
        };
        const float aspectRatios[] = {
            0.0f,
            16.0f / 9.0f,
            16.0f / 9.0f,
            16.0f / 10.0f,
            4.0f / 3.0f,
            21.0f / 9.0f
        };

        ImGui::SetNextItemWidth(165.0f);
        ImGui::Combo("##GameResolutionCombo", &m_selected_aspect_idx, aspectNames, IM_ARRAYSIZE(aspectNames));

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Game View");

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // ========================================================
        // 2. OYUN GÖRÜNÜMÜ RENDER ALANI (LETTERBOX / PILLARBOX)
        // ========================================================
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        const ImVec2 totalAvail = ImGui::GetContentRegionAvail();
        const ImVec2 startScreenPos = ImGui::GetCursorScreenPos();

        // Siyah arka planı doldur (Letterbox/Pillarbox barları için)
        ImDrawList* windowDl = ImGui::GetWindowDrawList();
        windowDl->AddRectFilled(
            startScreenPos,
            ImVec2(startScreenPos.x + totalAvail.x, startScreenPos.y + totalAvail.y),
            IM_COL32(10, 11, 14, 255)
        );

        ImVec2 renderSize = totalAvail;
        ImVec2 renderPos = startScreenPos;

        const float targetAspect = aspectRatios[m_selected_aspect_idx];
        if (targetAspect > 0.0f && totalAvail.x > 0.0f && totalAvail.y > 0.0f)
        {
            const float currentAspect = totalAvail.x / totalAvail.y;
            if (currentAspect > targetAspect)
            {
                // Geniş ekran: Sol ve sağda sütun (Pillarbox)
                renderSize.x = totalAvail.y * targetAspect;
                renderSize.y = totalAvail.y;
                renderPos.x += (totalAvail.x - renderSize.x) * 0.5f;
            }
            else
            {
                // Yüksek ekran: Üst ve altta şerit (Letterbox)
                renderSize.x = totalAvail.x;
                renderSize.y = totalAvail.x / targetAspect;
                renderPos.y += (totalAvail.y - renderSize.y) * 0.5f;
            }
        }

        if (renderSize.x > 1.0f && renderSize.y > 1.0f)
        {
            state.size = { renderSize.x, renderSize.y };
            state.position = { renderPos.x, renderPos.y };
        }

        unsigned int tex = m_vm.get_texture_id();

        if (tex != 0 && renderSize.x > 0.0f && renderSize.y > 0.0f)
        {
            ImGui::SetCursorScreenPos(renderPos);

            // OpenGL dokularının Y ekseni ters olduğu için UV koordinatlarını (0,1 -> 1,0) ters çevirerek çiz
            ImGui::Image(
                (ImTextureID)(intptr_t)tex,
                renderSize,
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            if (ImGui::IsItemClicked())
            {
                gameplay::input::set_cursor_locked(true);
            }

            // ========================================================
            // 3. IN-GAME UI RENDER VE ETKİLEŞİM KATMANI (CANVAS / RECTTRANSFORM)
            // ========================================================
            if (m_engine_ctx)
            {
                scene& currentScene = m_engine_ctx->session.game_scene();
                registry& reg = currentScene.get_registry();
                ImDrawList* dl = ImGui::GetWindowDrawList();

                // Çizimi oyun görünümü sınırlarına kırp (Clipping)
                dl->PushClipRect(renderPos, ImVec2(renderPos.x + renderSize.x, renderPos.y + renderSize.y), true);

                const ImVec2 mousePos = ImGui::GetIO().MousePos;
                const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

                for (entity e : reg.get_all_entities())
                {
                    if (!reg.has_component<rect_transform_component>(e))
                    {
                        continue;
                    }

                    // Canvas köküyse ve çizilecek öğesi yoksa atla
                    if (reg.has_component<canvas_component>(e) &&
                        !reg.has_component<ui_image_component>(e) &&
                        !reg.has_component<ui_text_component>(e))
                    {
                        continue;
                    }

                    auto* rt = reg.get_component<rect_transform_component>(e);
                    if (!rt || !rt->visible) continue;

                    // Ebeveyn (Parent) sınırlarını belirle
                    glm::vec4 parentRect(renderPos.x, renderPos.y, renderSize.x, renderSize.y);
                    if (auto* hc = reg.get_component<hierarchy_component>(e))
                    {
                        entity parent = hc->parent;
                        if (parent != 0 && reg.has_component<rect_transform_component>(parent) && !reg.has_component<canvas_component>(parent))
                        {
                            auto* parentRt = reg.get_component<rect_transform_component>(parent);
                            if (parentRt->computed_screen_rect.z > 0.0f && parentRt->computed_screen_rect.w > 0.0f)
                            {
                                parentRect = parentRt->computed_screen_rect;
                            }
                        }
                    }

                    // Unity Modeli Responsive Anchor & Pivot Hesaplaması
                    const float anchorMinX = parentRect.x + rt->anchor_min.x * parentRect.z;
                    const float anchorMaxX = parentRect.x + rt->anchor_max.x * parentRect.z;
                    const float anchorTopY = parentRect.y + (1.0f - rt->anchor_max.y) * parentRect.w;
                    const float anchorBottomY = parentRect.y + (1.0f - rt->anchor_min.y) * parentRect.w;

                    const float anchorW = anchorMaxX - anchorMinX;
                    const float anchorH = anchorBottomY - anchorTopY;

                    const glm::vec2 elemSize = rt->size * rt->scale;
                    const glm::vec2 pivotScreen(
                        anchorMinX + anchorW * rt->pivot.x + rt->anchored_position.x,
                        anchorTopY + anchorH * (1.0f - rt->pivot.y) - rt->anchored_position.y
                    );

                    const float x0 = pivotScreen.x - rt->pivot.x * elemSize.x;
                    const float y0 = pivotScreen.y - (1.0f - rt->pivot.y) * elemSize.y;
                    const float x1 = x0 + elemSize.x;
                    const float y1 = y0 + elemSize.y;

                    // Hesaplanmış mutlak ekran koordinatlarını bileşende önbellekle
                    rt->computed_screen_rect = glm::vec4(x0, y0, x1 - x0, y1 - y0);

                    const bool isHovered = (mousePos.x >= x0 && mousePos.x <= x1 && mousePos.y >= y0 && mousePos.y <= y1);

                    // A. UI Image / Box Çizimi
                    if (reg.has_component<ui_image_component>(e))
                    {
                        auto* img = reg.get_component<ui_image_component>(e);
                        glm::vec4 col = img->color;

                        // Eğer aynı varlıkta UI Button varsa durum rengini önceliklendir
                        if (reg.has_component<ui_button_component>(e))
                        {
                            auto* btn = reg.get_component<ui_button_component>(e);
                            if (!btn->interactable)
                            {
                                col = btn->disabled_color;
                                btn->current_state = ui_button_state::disabled;
                            }
                            else if (isHovered && mouseDown)
                            {
                                col = btn->pressed_color;
                                btn->current_state = ui_button_state::pressed;
                            }
                            else if (isHovered)
                            {
                                col = btn->hover_color;
                                btn->current_state = ui_button_state::hovered;
                            }
                            else
                            {
                                col = btn->normal_color;
                                btn->current_state = ui_button_state::normal;
                            }

                            if (isHovered && mouseClicked && btn->interactable && btn->on_click)
                            {
                                btn->on_click();
                            }
                        }

                        const ImU32 fillCol = ImColor(col.r, col.g, col.b, col.a);

                        bool renderedTex = false;
                        if (img->texture_handle.is_valid() && m_engine_ctx->aria_system)
                        {
                            aria::gpu_texture_id texId = m_engine_ctx->aria_system->get_texture_id(img->texture_handle);
                            if (texId != 0)
                            {
                                dl->AddImage((ImTextureID)(intptr_t)texId, ImVec2(x0, y0), ImVec2(x1, y1), ImVec2(0, 0), ImVec2(1, 1), fillCol);
                                renderedTex = true;
                            }
                        }

                        if (!renderedTex)
                        {
                            dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), fillCol, 4.0f);
                        }
                    }

                    // B. UI Text Çizimi
                    if (reg.has_component<ui_text_component>(e))
                    {
                        auto* txt = reg.get_component<ui_text_component>(e);
                        if (txt && !txt->text.empty())
                        {
                            const ImU32 textCol = ImColor(txt->color.r, txt->color.g, txt->color.b, txt->color.a);
                            ImFont* font = ImGui::GetFont();
                            const float fontSize = (txt->font_size > 0.0f) ? txt->font_size : ImGui::GetFontSize();

                            ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, txt->text.c_str());
                            ImVec2 textPos(x0, y0);

                            // Hizalama Hesaplaması
                            switch (txt->alignment)
                            {
                            case ui_text_alignment::top_left:
                                textPos = ImVec2(x0, y0);
                                break;
                            case ui_text_alignment::top_center:
                                textPos = ImVec2(x0 + (x1 - x0 - textSize.x) * 0.5f, y0);
                                break;
                            case ui_text_alignment::top_right:
                                textPos = ImVec2(x1 - textSize.x, y0);
                                break;
                            case ui_text_alignment::middle_left:
                                textPos = ImVec2(x0, y0 + (y1 - y0 - textSize.y) * 0.5f);
                                break;
                            case ui_text_alignment::center:
                                textPos = ImVec2(x0 + (x1 - x0 - textSize.x) * 0.5f, y0 + (y1 - y0 - textSize.y) * 0.5f);
                                break;
                            case ui_text_alignment::middle_right:
                                textPos = ImVec2(x1 - textSize.x, y0 + (y1 - y0 - textSize.y) * 0.5f);
                                break;
                            case ui_text_alignment::bottom_left:
                                textPos = ImVec2(x0, y1 - textSize.y);
                                break;
                            case ui_text_alignment::bottom_center:
                                textPos = ImVec2(x0 + (x1 - x0 - textSize.x) * 0.5f, y1 - textSize.y);
                                break;
                            case ui_text_alignment::bottom_right:
                                textPos = ImVec2(x1 - textSize.x, y1 - textSize.y);
                                break;
                            }

                            dl->AddText(font, fontSize, textPos, textCol, txt->text.c_str());
                        }
                    }
                }

                dl->PopClipRect();
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopID();
        ImGui::End();
    }

    const std::string& game_viewport_panel::get_title() const
    {
        return m_title;
    }

    bool game_viewport_panel::is_open() const
    {
        return m_is_open;
    }

    void game_viewport_panel::set_open(bool open)
    {
        m_is_open = open;
    }

} // namespace vesta