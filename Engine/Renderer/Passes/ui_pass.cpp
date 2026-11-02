/**
 * @file ui_pass.cpp
 * @brief UI render geçişi, standalone oyun içi Canvas/RectTransform çizimi ve ImGui çerçeve yaşam döngüsü uygulaması.
 */

#include "ui_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Editor/editor_context.h"
#include "../../ECS/components.h"
#include <imgui.h>
#include <cfloat>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace
{
    void render_standalone_canvas_ui(EngineContext& ctx)
    {
        scene& currentScene = ctx.session.game_scene();
        registry& reg = currentScene.get_registry();
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        const ImVec2 renderPos(0.0f, 0.0f);
        const ImVec2 renderSize = ImGui::GetIO().DisplaySize;
        if (renderSize.x <= 0.0f || renderSize.y <= 0.0f) return;

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

            if (reg.has_component<canvas_component>(e) &&
                !reg.has_component<ui_image_component>(e) &&
                !reg.has_component<ui_text_component>(e))
            {
                continue;
            }

            auto* rt = reg.get_component<rect_transform_component>(e);
            if (!rt || !rt->visible) continue;

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

            rt->computed_screen_rect = glm::vec4(x0, y0, x1 - x0, y1 - y0);

            const bool isHovered = (mousePos.x >= x0 && mousePos.x <= x1 && mousePos.y >= y0 && mousePos.y <= y1);

            if (reg.has_component<ui_image_component>(e))
            {
                auto* img = reg.get_component<ui_image_component>(e);
                glm::vec4 col = img->color;

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
                if (img->texture_handle.is_valid() && ctx.aria_system)
                {
                    aria::gpu_texture_id texId = ctx.aria_system->get_texture_id(img->texture_handle);
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

void ui_pass::render(EngineContext& ctx, EditorContext& editorCtx)
{
    if (!ctx.isEditorBuild)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, ctx.screenW, ctx.screenH);
        glDisable(GL_DEPTH_TEST);

        ctx.uiManager.beginStandaloneFrame();
        render_standalone_canvas_ui(ctx);
        ctx.uiManager.endFrame();

        glEnable(GL_DEPTH_TEST);
        return;
    }

    // 1. Ekran çerçeve tamponunu bağla ve tüm ekranı kapla
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.screenW, ctx.screenH);

    // 2. UI çizimi için derinlik testini devre dışı bırak ve tamponu temizle
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    // 3. Editör panellerinde gösterilecek FBO doku kimliklerini güncelle
    editorCtx.setSceneTexture(ctx.editorTarget.tex);
    editorCtx.setGameTexture(ctx.gameTarget.tex);

    // 4. ImGui ve Vesta arayüz çerçevesini çiz
    ctx.uiManager.beginFrame();

    ctx.vestaSys.update();

    ctx.uiManager.endFrame();

    // 5. Grafik boru hattı derinlik testini tekrar aktif et
    glEnable(GL_DEPTH_TEST);
}
