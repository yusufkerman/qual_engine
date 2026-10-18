#pragma once

/**
 * @file ui_components_ui.h
 * @brief 2B Oyun İçi UI bileşenleri (Canvas, RectTransform, Text, Image, Button) için Inspector arayüz ve operasyon kayıtları.
 * @note Hiçbir UI elemanı Canvas dışında yaşayamaz. Transform yerine RectTransform kullanılır.
 */

#include "../../Services/component_ui_registry.h"
#include "../../Services/component_operation_registry.h"
#include "../../Services/asset_entry.h"
#include "../../../ECS/registry.h"
#include "../../../ECS/components.h"
#include "../../../Engine/Assets/aria.h"
#include "../../../Engine/asset_pipeline.h"

#include <imgui.h>
#include <iostream>

namespace vesta {

    /**
     * @brief İki varlığı ECS hiyerarşisinde birbirine bağlar (Parent-Child).
     */
    inline void attach_child_to_parent(registry& reg, entity child, entity parent)
    {
        if (child == 0 || parent == 0 || child == parent) return;

        if (!reg.has_component<hierarchy_component>(parent)) {
            reg.add_component<hierarchy_component>(parent);
        }
        if (!reg.has_component<hierarchy_component>(child)) {
            reg.add_component<hierarchy_component>(child);
        }

        auto* child_hc = reg.get_component<hierarchy_component>(child);
        auto* parent_hc = reg.get_component<hierarchy_component>(parent);

        // Zaten bu ebeveyne bağlıysa işlem yapma
        if (child_hc->parent == parent) return;

        child_hc->parent = parent;
        child_hc->next_sibling = parent_hc->first_child;
        child_hc->prev_sibling = 0;

        if (parent_hc->first_child != 0) {
            auto* old_first_child_hc = reg.get_component<hierarchy_component>(parent_hc->first_child);
            if (old_first_child_hc) {
                old_first_child_hc->prev_sibling = child;
            }
        }
        parent_hc->first_child = child;
    }

    /**
     * @brief Sahne içinde geçerli bir Canvas varlığı arar; yoksa otomatik bir Canvas oluşturur.
     */
    inline entity ensure_canvas_exists(registry& reg)
    {
        // 1. Mevcut Canvas kontrolü
        for (entity e : reg.get_all_entities()) {
            if (reg.has_component<canvas_component>(e)) {
                return e;
            }
        }

        // 2. Yoksa yeni bir Canvas varlığı üret
        entity canvasEnt = reg.create_entity();
        reg.add_component<name_component>(canvasEnt, name_component{ "Canvas" });
        reg.add_component<canvas_component>(canvasEnt, canvas_component{});
        reg.add_component<rect_transform_component>(canvasEnt, rect_transform_component{
            glm::vec2(0.0f), glm::vec2(1920.0f, 1080.0f), glm::vec2(0.0f), glm::vec2(1.0f), glm::vec2(0.5f)
        });
        reg.add_component<hierarchy_component>(canvasEnt, hierarchy_component{});

        if (reg.has_component<transform_component>(canvasEnt)) {
            reg.remove_component<transform_component>(canvasEnt);
        }

        std::cout << "[Vesta UI] Sahneye otomatik Canvas olusturuldu (Entity ID: " << canvasEnt << ")\n";
        return canvasEnt;
    }

    /**
     * @brief UI elemanının 3B Transform içermemesini ve bir Canvas altında yaşamasını garanti eder.
     */
    inline void prepare_ui_entity(entity e, registry& reg, bool isCanvas = false)
    {
        // UI elemanları Transform taşımamalıdır
        if (reg.has_component<transform_component>(e)) {
            reg.remove_component<transform_component>(e);
        }

        if (isCanvas) return;

        // Varlık bir Canvas altında mı kontrol et
        bool hasCanvasAncestor = false;
        if (auto* hc = reg.get_component<hierarchy_component>(e)) {
            entity curr = hc->parent;
            while (curr != 0) {
                if (reg.has_component<canvas_component>(curr)) {
                    hasCanvasAncestor = true;
                    break;
                }
                auto* curr_hc = reg.get_component<hierarchy_component>(curr);
                curr = curr_hc ? curr_hc->parent : 0;
            }
        }

        if (!hasCanvasAncestor) {
            entity canvas = ensure_canvas_exists(reg);
            attach_child_to_parent(reg, e, canvas);
        }
    }

} // namespace vesta

// ============================================================================
// 1. CANVAS UI
// ============================================================================
inline void render_canvas_ui(entity e, registry& reg)
{
    auto* c = reg.get_component<canvas_component>(e);
    if (!c) return;

    const char* renderModes[] = { "Screen Space Overlay", "World Space" };
    int currentRenderMode = static_cast<int>(c->render_mode);
    if (ImGui::Combo("Render Mode", &currentRenderMode, renderModes, 2))
    {
        c->render_mode = static_cast<canvas_render_mode>(currentRenderMode);
    }

    const char* scalerModes[] = { "Constant Pixel Size", "Scale With Screen Size" };
    int currentScalerMode = static_cast<int>(c->scaler_mode);
    if (ImGui::Combo("UI Scale Mode", &currentScalerMode, scalerModes, 2))
    {
        c->scaler_mode = static_cast<canvas_scaler_mode>(currentScalerMode);
    }

    if (c->scaler_mode == canvas_scaler_mode::scale_with_screen_size)
    {
        ImGui::DragFloat2("Reference Res", &c->reference_resolution.x, 1.0f, 320.0f, 7680.0f, "%.0f");
        ImGui::SliderFloat("Match (W/H)", &c->match_width_or_height, 0.0f, 1.0f, "Width (0) <-> Height (1)");
    }

    ImGui::DragInt("Sorting Order", &c->sorting_order, 1, -100, 100);
}

inline void register_canvas_ui_system()
{
    size_t comp_hash = typeid(canvas_component).hash_code();

    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Canvas";
    ui_info.category = "UI";
    ui_info.render_fn = render_canvas_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    vesta::ComponentOperationInfo op_info;
    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<canvas_component>(e))
        {
            reg.add_component<canvas_component>(e);
            if (!reg.has_component<rect_transform_component>(e))
            {
                reg.add_component<rect_transform_component>(e);
            }
            vesta::prepare_ui_entity(e, reg, true);
        }
    };
    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<canvas_component>(e);
    };
    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<canvas_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, op_info);
}

// ============================================================================
// 2. RECT TRANSFORM UI
// ============================================================================
inline void render_rect_transform_ui(entity e, registry& reg)
{
    auto* rt = reg.get_component<rect_transform_component>(e);
    if (!rt) return;

    ImGui::Checkbox("Visible", &rt->visible);
    ImGui::DragFloat2("Anchored Pos", &rt->anchored_position.x, 1.0f, -10000.0f, 10000.0f, "%.1f");
    ImGui::DragFloat2("Size Delta", &rt->size.x, 1.0f, 1.0f, 10000.0f, "%.1f");

    if (ImGui::TreeNode("Anchors & Pivot"))
    {
        ImGui::SliderFloat2("Anchor Min", &rt->anchor_min.x, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat2("Anchor Max", &rt->anchor_max.x, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat2("Pivot", &rt->pivot.x, 0.0f, 1.0f, "%.2f");

        ImGui::Spacing();
        ImGui::TextDisabled("Quick Presets:");
        if (ImGui::SmallButton("Center"))
        {
            rt->anchor_min = glm::vec2(0.5f);
            rt->anchor_max = glm::vec2(0.5f);
            rt->pivot = glm::vec2(0.5f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Top-Left"))
        {
            rt->anchor_min = glm::vec2(0.0f, 1.0f);
            rt->anchor_max = glm::vec2(0.0f, 1.0f);
            rt->pivot = glm::vec2(0.0f, 1.0f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Top-Right"))
        {
            rt->anchor_min = glm::vec2(1.0f, 1.0f);
            rt->anchor_max = glm::vec2(1.0f, 1.0f);
            rt->pivot = glm::vec2(1.0f, 1.0f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Bottom-Left"))
        {
            rt->anchor_min = glm::vec2(0.0f, 0.0f);
            rt->anchor_max = glm::vec2(0.0f, 0.0f);
            rt->pivot = glm::vec2(0.0f, 0.0f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Stretch All"))
        {
            rt->anchor_min = glm::vec2(0.0f, 0.0f);
            rt->anchor_max = glm::vec2(1.0f, 1.0f);
            rt->pivot = glm::vec2(0.5f, 0.5f);
        }

        ImGui::TreePop();
    }

    ImGui::DragFloat("Rotation Z", &rt->rotation, 0.5f, -360.0f, 360.0f, "%.1f deg");
    ImGui::DragFloat2("Scale", &rt->scale.x, 0.05f, 0.01f, 10.0f, "%.2f");
}

inline void register_rect_transform_ui_system()
{
    size_t comp_hash = typeid(rect_transform_component).hash_code();

    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "Rect Transform";
    ui_info.category = "UI";
    ui_info.render_fn = render_rect_transform_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    vesta::ComponentOperationInfo op_info;
    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<rect_transform_component>(e))
        {
            reg.add_component<rect_transform_component>(e);
            vesta::prepare_ui_entity(e, reg, reg.has_component<canvas_component>(e));
        }
    };
    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<rect_transform_component>(e);
    };
    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<rect_transform_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, op_info);
}

// ============================================================================
// 3. UI TEXT UI
// ============================================================================
inline void render_ui_text_ui(entity e, registry& reg)
{
    auto* t = reg.get_component<ui_text_component>(e);
    if (!t) return;

    char textBuf[512];
    strncpy_s(textBuf, t->text.c_str(), sizeof(textBuf));
    if (ImGui::InputTextMultiline("Text Content", textBuf, sizeof(textBuf), ImVec2(-1.0f, 60.0f)))
    {
        t->text = textBuf;
    }

    ImGui::ColorEdit4("Text Color", &t->color.x);
    ImGui::DragFloat("Font Size", &t->font_size, 0.5f, 8.0f, 128.0f, "%.0f px");

    const char* alignmentOptions[] = {
        "Top Left", "Top Center", "Top Right",
        "Middle Left", "Center", "Middle Right",
        "Bottom Left", "Bottom Center", "Bottom Right"
    };
    int currentAlign = static_cast<int>(t->alignment);
    if (ImGui::Combo("Alignment", &currentAlign, alignmentOptions, 9))
    {
        t->alignment = static_cast<ui_text_alignment>(currentAlign);
    }

    ImGui::Checkbox("Word Wrap", &t->wrap_text);
    ImGui::DragFloat("Line Spacing", &t->line_spacing, 0.05f, 0.5f, 3.0f, "%.2f");
}

inline void register_ui_text_ui_system()
{
    size_t comp_hash = typeid(ui_text_component).hash_code();

    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "UI Text";
    ui_info.category = "UI";
    ui_info.render_fn = render_ui_text_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    vesta::ComponentOperationInfo op_info;
    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<ui_text_component>(e))
        {
            reg.add_component<ui_text_component>(e);
            if (!reg.has_component<rect_transform_component>(e))
            {
                reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(160.0f, 40.0f) });
            }
            vesta::prepare_ui_entity(e, reg, false);
        }
    };
    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<ui_text_component>(e);
    };
    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<ui_text_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, op_info);
}

// ============================================================================
// 4. UI IMAGE UI
// ============================================================================
inline void render_ui_image_ui(entity e, registry& reg, aria::system* aria_sys)
{
    auto* img = reg.get_component<ui_image_component>(e);
    if (!img) return;

    ImGui::ColorEdit4("Image Color", &img->color.x);
    ImGui::Checkbox("Raycast Target", &img->raycast_target);

    ImGui::Spacing();
    ImGui::Text("Source Image (Texture):");

    std::string slotName = "None (Drop Texture Here)";
    if (img->texture_handle.is_valid())
    {
        slotName = "Assigned Texture Asset";
    }

    ImGui::PushStyleColor(ImGuiCol_Button, img->texture_handle.is_valid() ? ImVec4(0.18f, 0.40f, 0.20f, 1.0f) : ImVec4(0.22f, 0.22f, 0.25f, 1.0f));
    ImGui::Button(slotName.c_str(), ImVec2(-1.0f, 32.0f));
    ImGui::PopStyleColor();

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARIA_ASSET_PAYLOAD"))
        {
            const auto* data = static_cast<const vesta::asset_drag_payload*>(payload->Data);
            if (data && data->type == vesta::asset_payload_type::texture && aria_sys)
            {
                aria::handle<aria::texture_asset> imported = aria_sys->import_texture(data->guid, data->path);
                if (imported.is_valid())
                {
                    img->texture_handle = imported;
                    std::cout << "[Vesta UI] Doku UI Image bilesenine atandi: " << data->path << std::endl;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (img->texture_handle.is_valid() && aria_sys)
    {
        aria::gpu_texture_id texId = aria_sys->get_texture_id(img->texture_handle);
        if (texId != 0)
        {
            ImGui::Image((ImTextureID)(intptr_t)texId, ImVec2(56.0f, 56.0f));
            ImGui::SameLine();
        }
        if (ImGui::SmallButton("Remove Texture"))
        {
            img->texture_handle = {};
        }
    }
}

inline void register_ui_image_ui_system(aria::system* aria_sys)
{
    size_t comp_hash = typeid(ui_image_component).hash_code();

    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "UI Image";
    ui_info.category = "UI";
    ui_info.render_fn = [aria_sys](entity e, registry& reg) {
        render_ui_image_ui(e, reg, aria_sys);
    };
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    vesta::ComponentOperationInfo op_info;
    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<ui_image_component>(e))
        {
            reg.add_component<ui_image_component>(e);
            if (!reg.has_component<rect_transform_component>(e))
            {
                reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(100.0f, 100.0f) });
            }
            vesta::prepare_ui_entity(e, reg, false);
        }
    };
    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<ui_image_component>(e);
    };
    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<ui_image_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, op_info);
}

// ============================================================================
// 5. UI BUTTON UI
// ============================================================================
inline void render_ui_button_ui(entity e, registry& reg)
{
    auto* btn = reg.get_component<ui_button_component>(e);
    if (!btn) return;

    ImGui::Checkbox("Interactable", &btn->interactable);
    ImGui::ColorEdit4("Normal Color", &btn->normal_color.x);
    ImGui::ColorEdit4("Hover Color", &btn->hover_color.x);
    ImGui::ColorEdit4("Pressed Color", &btn->pressed_color.x);
    ImGui::ColorEdit4("Disabled Color", &btn->disabled_color.x);

    const char* stateNames[] = { "Normal", "Hovered", "Pressed", "Disabled" };
    ImGui::TextDisabled("State: %s", stateNames[static_cast<int>(btn->current_state)]);
}

inline void register_ui_button_ui_system()
{
    size_t comp_hash = typeid(ui_button_component).hash_code();

    vesta::ComponentUIInfo ui_info;
    ui_info.display_name = "UI Button";
    ui_info.category = "UI";
    ui_info.render_fn = render_ui_button_ui;
    vesta::component_ui_registry::register_component(comp_hash, ui_info);

    vesta::ComponentOperationInfo op_info;
    op_info.add_fn = [](entity e, registry& reg) {
        if (!reg.has_component<ui_button_component>(e))
        {
            reg.add_component<ui_button_component>(e);
            if (!reg.has_component<rect_transform_component>(e))
            {
                reg.add_component<rect_transform_component>(e, rect_transform_component{ glm::vec2(0.0f), glm::vec2(160.0f, 48.0f) });
            }
            if (!reg.has_component<ui_image_component>(e))
            {
                reg.add_component<ui_image_component>(e);
            }
            vesta::prepare_ui_entity(e, reg, false);

            // Unity tarzı alt çocuk nesnesi: "Text"
            entity textChild = reg.create_entity();
            reg.add_component<name_component>(textChild, name_component{ "Text" });
            if (reg.has_component<transform_component>(textChild))
            {
                reg.remove_component<transform_component>(textChild);
            }
            reg.add_component<rect_transform_component>(textChild, rect_transform_component{
                glm::vec2(0.0f), glm::vec2(160.0f, 48.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.5f, 0.5f)
            });
            reg.add_component<ui_text_component>(textChild, ui_text_component{
                "Button", "EngineResources/Fonts/Roboto-Regular.ttf", 20.0f, glm::vec4(0.12f, 0.12f, 0.12f, 1.0f)
            });
            vesta::attach_child_to_parent(reg, textChild, e);
        }
    };
    op_info.remove_fn = [](entity e, registry& reg) {
        reg.remove_component<ui_button_component>(e);
    };
    op_info.has_fn = [](entity e, registry& reg) -> bool {
        return reg.has_component<ui_button_component>(e);
    };
    vesta::component_operation_registry::register_component(comp_hash, op_info);
}
