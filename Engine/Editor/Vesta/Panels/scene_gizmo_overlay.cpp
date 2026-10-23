/**
 * @file scene_gizmo_overlay.cpp
 * @brief Sahne görünümü (Scene Viewport) etkileşimli Transform araçları, Işık menzil/koni tutamaçları,
 *        Billboard ikonları ve Gizmo ayar araç çubuğu orkestrasyon uygulaması.
 */

#include "scene_gizmo_overlay.h"
#include "../Gizmo/gizmo_math_utils.h"
#include "../../../Engine/engine_context.h"
#include "../../../Editor/editor_context.h"
#include "../../../Editor/editor_camera.h"
#include "../vesta.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"
#include "../../../Renderer/MeshRenderer.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdarg>

namespace vesta
{
    namespace
    {
        /**
         * @brief Görünürlük modu için ImGui Combo seçici çizer.
         */
        void draw_visibility_combo(const char* label, gizmo_visibility_mode& mode)
        {
            const char* items[] = { "Off", "Selected Only", "Always (All)" };
            int current = static_cast<int>(mode);
            ImGui::SetNextItemWidth(135.0f);
            if (ImGui::Combo(label, &current, items, IM_ARRAYSIZE(items)))
            {
                mode = static_cast<gizmo_visibility_mode>(current);
            }
        }
    } // anonymous namespace

    // ========================================================================
    // İKON ÇÖZÜMLEME VE VEKTÖREL İKON ÇİZİMİ (PUBLIC STATICS)
    // ========================================================================

    entity_icon_type scene_gizmo_overlay::resolve_entity_icon(
        entity e,
        const registry& reg,
        const gizmo_settings& settings)
    {
        const entity_icon_type custom = settings.get_entity_icon(e);
        if (custom != entity_icon_type::auto_detect)
        {
            return custom;
        }

        if (reg.has_component<audio_source_component>(e))
        {
            return entity_icon_type::audio_source;
        }

        if (reg.has_component<camera_component>(e))
        {
            return entity_icon_type::camera;
        }

        if (const auto* light = reg.get_component<light_component>(e))
        {
            switch (light->type)
            {
            case light_type::directional: return entity_icon_type::directional_light;
            case light_type::point:       return entity_icon_type::point_light;
            case light_type::spot:        return entity_icon_type::spot_light;
            default:                      return entity_icon_type::point_light;
            }
        }

        return entity_icon_type::none;
    }

    const char* scene_gizmo_overlay::get_icon_type_name(entity_icon_type type)
    {
        switch (type)
        {
        case entity_icon_type::auto_detect:       return "Auto (Default)";
        case entity_icon_type::none:              return "None (Hidden)";
        case entity_icon_type::camera:            return "Camera";
        case entity_icon_type::directional_light: return "Directional Light (Sun)";
        case entity_icon_type::point_light:       return "Point Light (Bulb)";
        case entity_icon_type::spot_light:        return "Spot Light (Projector)";
        case entity_icon_type::spawn_point:       return "Spawn Point (Flag)";
        case entity_icon_type::trigger_zone:      return "Trigger Zone (Diamond)";
        case entity_icon_type::audio_source:      return "Audio Source (Speaker)";
        case entity_icon_type::waypoint:          return "Waypoint (Crosshair)";
        case entity_icon_type::label_blue:        return "Label: Blue";
        case entity_icon_type::label_green:       return "Label: Green";
        case entity_icon_type::label_yellow:      return "Label: Yellow";
        case entity_icon_type::label_orange:      return "Label: Orange";
        case entity_icon_type::label_red:         return "Label: Red";
        case entity_icon_type::label_purple:      return "Label: Purple";
        default:                                  return "Auto";
        }
    }

    void scene_gizmo_overlay::draw_icon_badge_2d(
        entity_icon_type type,
        const glm::vec2& center,
        float radius,
        const glm::vec3& customTint)
    {
        if (type == entity_icon_type::none || type == entity_icon_type::auto_detect) {
            return;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 c(center.x, center.y);
        const float r = std::max(5.0f, radius);

        auto toCol = [](float cr, float cg, float cb, float ca = 1.0f) -> ImU32 {
            return ImGui::ColorConvertFloat4ToU32(ImVec4(cr, cg, cb, ca));
        };

        const ImU32 bgCol = toCol(0.12f, 0.14f, 0.18f, 0.88f);
        const ImU32 borderCol = toCol(0.05f, 0.05f, 0.06f, 0.95f);
        dl->AddCircleFilled(c, r, bgCol, 20);
        dl->AddCircle(c, r, borderCol, 20, 1.4f);

        auto drawTintedCircle = [&](float tintR, float tintG, float tintB)
        {
            const ImU32 fill = toCol(tintR, tintG, tintB, 0.95f);
            dl->AddCircleFilled(c, r * 0.65f, fill, 16);
            dl->AddCircle(c, r * 0.65f, toCol(0.0f, 0.0f, 0.0f, 0.8f), 16, 1.0f);
        };

        switch (type)
        {
        case entity_icon_type::label_blue:   drawTintedCircle(0.25f, 0.55f, 1.0f);  return;
        case entity_icon_type::label_green:  drawTintedCircle(0.28f, 0.88f, 0.35f); return;
        case entity_icon_type::label_yellow: drawTintedCircle(1.00f, 0.88f, 0.20f); return;
        case entity_icon_type::label_orange: drawTintedCircle(1.00f, 0.55f, 0.15f); return;
        case entity_icon_type::label_red:    drawTintedCircle(0.95f, 0.25f, 0.25f); return;
        case entity_icon_type::label_purple: drawTintedCircle(0.70f, 0.35f, 0.95f); return;
        default: break;
        }

        const ImU32 iconCol = toCol(customTint.r, customTint.g, customTint.b, 0.95f);

        switch (type)
        {
        case entity_icon_type::camera:
        {
            const float bw = r * 0.55f;
            const float bh = r * 0.38f;
            dl->AddRectFilled(ImVec2(c.x - bw, c.y - bh * 0.6f), ImVec2(c.x + bw * 0.4f, c.y + bh), iconCol, 1.5f);
            const ImVec2 tip(c.x + bw, c.y + bh * 0.2f);
            const ImVec2 t1(c.x + bw * 0.4f, c.y - bh * 0.4f);
            const ImVec2 t2(c.x + bw * 0.4f, c.y + bh * 0.8f);
            dl->AddTriangleFilled(tip, t1, t2, iconCol);
            break;
        }
        case entity_icon_type::directional_light:
        {
            const float cr = r * 0.32f;
            dl->AddCircleFilled(c, cr, iconCol, 12);
            for (int i = 0; i < 8; ++i)
            {
                const float a = static_cast<float>(i) * (glm::two_pi<float>() / 8.0f);
                const float s = std::sin(a), cs = std::cos(a);
                const ImVec2 p0(c.x + cs * (cr * 1.35f), c.y + s * (cr * 1.35f));
                const ImVec2 p1(c.x + cs * (r * 0.75f), c.y + s * (r * 0.75f));
                dl->AddLine(p0, p1, iconCol, 1.4f);
            }
            break;
        }
        case entity_icon_type::point_light:
        {
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.12f), r * 0.42f, iconCol, 12);
            dl->AddRectFilled(ImVec2(c.x - r * 0.22f, c.y + r * 0.22f), ImVec2(c.x + r * 0.22f, c.y + r * 0.55f), iconCol, 1.0f);
            break;
        }
        case entity_icon_type::spot_light:
        {
            const ImVec2 apex(c.x, c.y - r * 0.45f);
            const ImVec2 bl(c.x - r * 0.55f, c.y + r * 0.45f);
            const ImVec2 br(c.x + r * 0.55f, c.y + r * 0.45f);
            dl->AddTriangleFilled(apex, bl, br, iconCol);
            dl->AddLine(bl, br, iconCol, 1.6f);
            break;
        }
        case entity_icon_type::spawn_point:
        {
            dl->AddLine(ImVec2(c.x - r * 0.25f, c.y + r * 0.55f), ImVec2(c.x - r * 0.25f, c.y - r * 0.55f), iconCol, 1.8f);
            const ImVec2 f0(c.x - r * 0.25f, c.y - r * 0.55f);
            const ImVec2 f1(c.x + r * 0.5f,  c.y - r * 0.25f);
            const ImVec2 f2(c.x - r * 0.25f, c.y + r * 0.05f);
            dl->AddTriangleFilled(f0, f1, f2, iconCol);
            break;
        }
        case entity_icon_type::trigger_zone:
        {
            const ImVec2 d0(c.x, c.y - r * 0.6f);
            const ImVec2 d1(c.x + r * 0.6f, c.y);
            const ImVec2 d2(c.x, c.y + r * 0.6f);
            const ImVec2 d3(c.x - r * 0.6f, c.y);
            dl->AddQuadFilled(d0, d1, d2, d3, iconCol);
            dl->AddQuad(d0, d1, d2, d3, toCol(0.1f, 0.1f, 0.1f, 0.9f), 1.0f);
            break;
        }
        case entity_icon_type::audio_source:
        {
            dl->AddRectFilled(ImVec2(c.x - r * 0.45f, c.y - r * 0.25f), ImVec2(c.x - r * 0.15f, c.y + r * 0.25f), iconCol);
            const ImVec2 cone0(c.x - r * 0.15f, c.y - r * 0.25f);
            const ImVec2 cone1(c.x + r * 0.15f, c.y - r * 0.5f);
            const ImVec2 cone2(c.x + r * 0.15f, c.y + r * 0.5f);
            const ImVec2 cone3(c.x - r * 0.15f, c.y + r * 0.25f);
            dl->AddQuadFilled(cone0, cone1, cone2, cone3, iconCol);
            for (int w = 1; w <= 2; ++w)
            {
                const float waveR = r * 0.35f * static_cast<float>(w);
                dl->PathArcTo(ImVec2(c.x - r * 0.1f, c.y), waveR, -glm::quarter_pi<float>(), glm::quarter_pi<float>(), 8);
                dl->PathStroke(iconCol, 0, 1.4f);
            }
            break;
        }
        case entity_icon_type::waypoint:
        {
            dl->AddCircle(c, r * 0.45f, iconCol, 12, 1.5f);
            dl->AddLine(ImVec2(c.x - r * 0.65f, c.y), ImVec2(c.x + r * 0.65f, c.y), iconCol, 1.3f);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.65f), ImVec2(c.x, c.y + r * 0.65f), iconCol, 1.3f);
            break;
        }
        default:
            dl->AddCircleFilled(c, r * 0.45f, iconCol, 12);
            break;
        }
    }

    // ========================================================================
    // ANA RENDER ORKESTRASYONU
    // ========================================================================

    void scene_gizmo_overlay::render(
        EngineContext& engineCtx,
        EditorContext& editorCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize,
        bool isViewportFocused,
        bool isViewportHovered)
    {
        if (viewportSize.x < 10.0f || viewportSize.y < 10.0f || !editorCtx.camera) {
            return;
        }

        // 1. Kısayol Tuşlarını İşle (Q, W, E, R, T, X, G)
        process_shortcuts(engineCtx, editorCtx, isViewportFocused, isViewportHovered);

        const float aspect = viewportSize.x / std::max(1.0f, viewportSize.y);
        const glm::mat4 view = editorCtx.camera->getViewMatrix();
        const glm::mat4 proj = editorCtx.camera->getProjectionMatrix(aspect);
        const glm::vec3 camPos = editorCtx.camera->position;

        auto& settings = engineCtx.gizmoSystem.settings();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(
            ImVec2(viewportPos.x, viewportPos.y),
            ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y),
            true
        );

        if (settings.enabled)
        {
            // 2. Etkileşimli Işık Menzil ve Koni Tutamaçları (Point & Spot Light)
            if (settings.interactive_light_handles && settings.light_gizmo_mode != gizmo_visibility_mode::off)
            {
                m_LightTool.render(engineCtx, view, proj, camPos, viewportPos, viewportSize);
            }

            // 2b. Etkileşimli Ses Menzil Tutamaçları (Audio Source Min/Max Distance)
            if (settings.interactive_audio_handles && settings.audio_gizmo_mode != gizmo_visibility_mode::off)
            {
                m_AudioTool.render(engineCtx, view, proj, camPos, viewportPos, viewportSize);
            }

            // 3. UI Katmanı (Canvas Sınırları, Metinler, Görseller ve UI Sınır Çizgileri)
            render_ui_overlay(engineCtx, viewportPos, viewportSize);

            // 4. Etkileşimli 3B Dönüşüm Gizmosu veya 2B UI Rect Tool
            if (settings.active_tool == gizmo_tool_mode::rect_tool)
            {
                m_RectTool.render(engineCtx, viewportPos, viewportSize);
            }
            else
            {
                render_transform_gizmo(
                    engineCtx,
                    view,
                    proj,
                    camPos,
                    editorCtx.camera->fov,
                    viewportPos,
                    viewportSize
                );
            }

            // 5. Görünmez Nesneler (Kamera, Işıklar ve Özel İkonlu Varlıklar) için Billboard İkonlar
            if (settings.show_icons)
            {
                const bool allowIconClick = isViewportHovered && !is_interacting();
                render_billboard_icons(engineCtx, view, proj, camPos, viewportPos, viewportSize, allowIconClick);
            }
        }

        dl->PopClipRect();

        // 6. Üst Araç Çubuğu ve Gizmos Ayar Menüsü (Her zaman erişilebilir)
        render_toolbar(engineCtx, editorCtx, viewportPos, viewportSize);

        // 7. Gerçek Zamanlı Performans ve Render İstatistikleri Penceresi (Stats Overlay)
        if (settings.show_stats_overlay)
        {
            render_stats_overlay(engineCtx, editorCtx, viewportPos, viewportSize);
        }
    }

    void scene_gizmo_overlay::process_shortcuts(
        EngineContext& engineCtx,
        EditorContext& editorCtx,
        bool isViewportFocused,
        bool isViewportHovered)
    {
        (void)editorCtx;
        if (!isViewportFocused && !isViewportHovered) {
            return;
        }

        // Sağ fare tuşu basılıyken W/A/S/D uçuş kamerasını (Fly Camera) kontrol eder; kısayolları devre dışı bırak
        if (engineCtx.window && glfwGetMouseButton(engineCtx.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            return;
        }

        if (ImGui::GetIO().WantTextInput) {
            return;
        }

        auto& settings = engineCtx.gizmoSystem.settings();

        if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
            settings.active_tool = gizmo_tool_mode::select;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
            settings.active_tool = gizmo_tool_mode::translate;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
            settings.active_tool = gizmo_tool_mode::rotate;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            settings.active_tool = gizmo_tool_mode::scale;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
            settings.active_tool = gizmo_tool_mode::rect_tool;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
            settings.transform_space = (settings.transform_space == gizmo_space::local)
                ? gizmo_space::world
                : gizmo_space::local;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_G, false)) {
            settings.show_grid = !settings.show_grid;
        }
    }

    void scene_gizmo_overlay::render_toolbar(
        EngineContext& engineCtx,
        EditorContext& editorCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        (void)editorCtx;
        auto& settings = engineCtx.gizmoSystem.settings();

        const float pad = 10.0f;
        ImGui::SetCursorScreenPos(ImVec2(viewportPos.x + pad, viewportPos.y + pad));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 0.88f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

        const float barWidth = std::min(viewportSize.x - 2.0f * pad, 860.0f);
        if (ImGui::BeginChild("##SceneGizmoToolbar", ImVec2(barWidth, 32.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            auto drawToolBtn = [&](const char* label, gizmo_tool_mode mode, const char* tooltip)
            {
                const bool active = (settings.active_tool == mode);
                if (active) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.52f, 0.88f, 0.95f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.58f, 0.95f, 1.0f));
                }
                if (ImGui::SmallButton(label)) {
                    settings.active_tool = mode;
                }
                if (active) {
                    ImGui::PopStyleColor(2);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", tooltip);
                }
                ImGui::SameLine(0.0f, 4.0f);
            };

            drawToolBtn("Q: Select", gizmo_tool_mode::select, "Select Tool (Shortcut: Q)");
            drawToolBtn("W: Move",   gizmo_tool_mode::translate, "Move Tool (Shortcut: W)");
            drawToolBtn("E: Rotate", gizmo_tool_mode::rotate, "Rotate Tool (Shortcut: E)");
            drawToolBtn("R: Scale",  gizmo_tool_mode::scale, "Scale Tool (Shortcut: R)");
            drawToolBtn("T: Rect",   gizmo_tool_mode::rect_tool, "2D UI Rect Tool (Shortcut: T)");

            ImGui::TextDisabled("|");
            ImGui::SameLine(0.0f, 6.0f);

            // Local / World Coordinate Space Toggle
            const char* spaceLabel = (settings.transform_space == gizmo_space::local) ? "Space: Local" : "Space: World";
            if (ImGui::SmallButton(spaceLabel))
            {
                settings.transform_space = (settings.transform_space == gizmo_space::local)
                    ? gizmo_space::world
                    : gizmo_space::local;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle Coordinate Space: Local / World (Shortcut: X)");
            }
            ImGui::SameLine(0.0f, 4.0f);

            // Snapping Button
            const bool snapActive = settings.snap_enabled;
            if (snapActive) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.65f, 0.40f, 0.95f));
            }
            if (ImGui::SmallButton("Snap")) {
                settings.snap_enabled = !settings.snap_enabled;
            }
            if (snapActive) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle Snapping. Hold Ctrl while dragging for temporary snap.");
            }
            ImGui::SameLine(0.0f, 6.0f);

            ImGui::TextDisabled("|");
            ImGui::SameLine(0.0f, 6.0f);

            // Quick Grid Button
            const bool gridOn = settings.show_grid;
            if (gridOn) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.34f, 0.45f, 0.95f));
            }
            if (ImGui::SmallButton("Grid")) {
                settings.show_grid = !settings.show_grid;
            }
            if (gridOn) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle Infinite Reference Grid (Shortcut: G)");
            }
            ImGui::SameLine(0.0f, 4.0f);

            // Quick 3D Icons Button
            const bool iconsOn = settings.show_icons;
            if (iconsOn) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.34f, 0.45f, 0.95f));
            }
            if (ImGui::SmallButton("Icons")) {
                settings.show_icons = !settings.show_icons;
            }
            if (iconsOn) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle 3D Billboard Icons");
            }
            ImGui::SameLine(0.0f, 4.0f);

            // Quick Scene Fog Toggle Button
            const bool fogOn = engineCtx.fog.enabled && engineCtx.fog.show_in_scene;
            if (fogOn) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.34f, 0.45f, 0.95f));
            }
            if (ImGui::SmallButton("Fog")) {
                engineCtx.fog.show_in_scene = !engineCtx.fog.show_in_scene;
            }
            if (fogOn) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle Scene View Fog (Currently: %s)", engineCtx.fog.show_in_scene ? "Enabled" : "Disabled");
            }
            ImGui::SameLine(0.0f, 4.0f);

            // Quick Scene Stats Overlay Button
            const bool statsOn = settings.show_stats_overlay;
            if (statsOn) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.52f, 0.88f, 0.95f));
            }
            if (ImGui::SmallButton("Stats")) {
                settings.show_stats_overlay = !settings.show_stats_overlay;
            }
            if (statsOn) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle Real-Time Performance & Render Statistics Overlay");
            }
            ImGui::SameLine(0.0f, 6.0f);

            // Comprehensive Gizmos Settings Popup Menu
            if (ImGui::SmallButton("Gizmos v"))
            {
                ImGui::OpenPopup("SceneGizmosSettingsPopup");
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Gizmos & Viewport Settings");
            }

            if (ImGui::BeginPopup("SceneGizmosSettingsPopup"))
            {
                ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "Scene Gizmos & Viewport Settings");
                ImGui::Separator();

                ImGui::Checkbox("Enable All 3D Gizmos", &settings.enabled);
                ImGui::SameLine();
                ImGui::Checkbox("Show Stats Overlay", &settings.show_stats_overlay);
                ImGui::SliderFloat("Transform Gizmo Size", &settings.handle_size, 55.0f, 160.0f, "%.0f px");
                ImGui::SliderFloat("Wireframe Line Width", &settings.line_width, 1.0f, 4.5f, "%.1f px");

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Infinite Reference Grid");
                ImGui::Checkbox("Show Grid (G)", &settings.show_grid);
                ImGui::SameLine();
                ImGui::Checkbox("Highlight X/Z Axes", &settings.show_grid_axes);
                ImGui::SliderFloat("Grid Opacity", &settings.grid_opacity, 0.10f, 1.0f, "%.2f");

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Billboard Icons & Hierarchy Badges");
                ImGui::Checkbox("Show 3D Scene Icons", &settings.show_icons);
                ImGui::SameLine();
                ImGui::Checkbox("Distance Scaling", &settings.icon_distance_scaling);
                ImGui::SliderFloat("Icon Size", &settings.icon_size, 0.35f, 2.50f, "%.2fx");
                ImGui::Checkbox("Show Hierarchy Icon Badges", &settings.show_hierarchy_badges);

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Component Gizmos Visibility");
                draw_visibility_combo("Colliders", settings.collider_mode);
                draw_visibility_combo("Camera Frustum", settings.camera_frustum_mode);
                ImGui::Checkbox("Clamp Camera Frustum Preview", &settings.clamp_camera_frustum_dist);
                if (settings.clamp_camera_frustum_dist)
                {
                    ImGui::SliderFloat("Max Frustum Preview Dist", &settings.camera_frustum_max_draw_dist, 5.0f, 150.0f, "%.1f m");
                }

                draw_visibility_combo("Lights (Range & Cone)", settings.light_gizmo_mode);
                ImGui::Checkbox("Interactive Light Range/Cone Handles", &settings.interactive_light_handles);

                draw_visibility_combo("Audio (Min/Max Distance)", settings.audio_gizmo_mode);
                ImGui::Checkbox("Interactive Audio Range Handles", &settings.interactive_audio_handles);

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Snap Settings");
                ImGui::Checkbox("Enable Snapping", &settings.snap_enabled);
                ImGui::DragFloat("Move Snap", &settings.translate_snap, 0.05f, 0.05f, 50.0f, "%.2f m");
                ImGui::DragFloat("Rotate Snap", &settings.rotate_snap_deg, 1.0f, 1.0f, 90.0f, "%.1f deg");
                ImGui::DragFloat("Scale Snap", &settings.scale_snap, 0.05f, 0.05f, 10.0f, "%.2f");

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Gameplay Debug Gizmos");
                ImGui::Checkbox("Show in Scene View", &settings.show_gameplay_gizmos_in_scene);
                ImGui::SameLine();
                ImGui::Checkbox("Show in Game View", &settings.show_gameplay_gizmos_in_game);

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.85f, 0.88f, 0.95f, 1.0f), "Atmosphere & Fog");
                ImGui::Checkbox("Show Fog in Scene View", &engineCtx.fog.show_in_scene);
                ImGui::SameLine();
                ImGui::Checkbox("Fog Master Enabled", &engineCtx.fog.enabled);

                ImGui::Separator();
                if (ImGui::Button("Reset Gizmo Defaults", ImVec2(-1.0f, 0.0f)))
                {
                    settings.reset_defaults();
                }

                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }

    void scene_gizmo_overlay::render_stats_overlay(
        EngineContext& engineCtx,
        EditorContext& editorCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        auto& settings = engineCtx.gizmoSystem.settings();
        const ImGuiIO& io = ImGui::GetIO();

        const float rawDt = std::max(0.0001f, io.DeltaTime);
        const float instantFps = io.Framerate > 0.0f ? io.Framerate : (1.0f / rawDt);
        const float instantMs = rawDt * 1000.0f;

        m_SmoothedFps = glm::mix(m_SmoothedFps, instantFps, 0.12f);
        m_SmoothedFrameMs = glm::mix(m_SmoothedFrameMs, instantMs, 0.12f);

        m_FrameTimeHistoryMs[m_FrameHistoryOffset] = instantMs;
        m_FrameHistoryOffset = (m_FrameHistoryOffset + 1) % kFrameHistoryCount;

        float maxHistoryMs = 0.0f;
        float minHistoryMs = 9999.0f;
        for (int i = 0; i < kFrameHistoryCount; ++i)
        {
            if (m_FrameTimeHistoryMs[i] > 0.0f)
            {
                maxHistoryMs = std::max(maxHistoryMs, m_FrameTimeHistoryMs[i]);
                minHistoryMs = std::min(minHistoryMs, m_FrameTimeHistoryMs[i]);
            }
        }
        if (minHistoryMs > maxHistoryMs) minHistoryMs = 0.0f;

        const float overlayW = std::min(285.0f, std::max(180.0f, viewportSize.x - 20.0f));
        const float overlayH = std::min(305.0f, std::max(120.0f, viewportSize.y - 60.0f));
        const float posX = viewportPos.x + std::max(10.0f, viewportSize.x - overlayW - 12.0f);
        const float posY = viewportPos.y + 48.0f;

        ImGui::SetCursorScreenPos(ImVec2(posX, posY));

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.08f, 0.11f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.36f, 0.48f, 0.70f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 7.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));

        if (ImGui::BeginChild("##SceneStatsOverlay", ImVec2(overlayW, overlayH), true, ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f), "Scene & Render Stats");
            ImGui::SameLine(overlayW - 28.0f);
            if (ImGui::SmallButton("x"))
            {
                settings.show_stats_overlay = false;
            }
            ImGui::Separator();

            ImVec4 fpsColor(0.32f, 0.95f, 0.52f, 1.0f);
            if (m_SmoothedFps < 30.0f) {
                fpsColor = ImVec4(0.96f, 0.32f, 0.32f, 1.0f);
            }
            else if (m_SmoothedFps < 55.0f) {
                fpsColor = ImVec4(0.98f, 0.80f, 0.24f, 1.0f);
            }

            ImGui::Text("FPS:");
            ImGui::SameLine(115.0f);
            ImGui::TextColored(fpsColor, "%.1f FPS (%.2f ms)", m_SmoothedFps, m_SmoothedFrameMs);

            char overlayBuf[64];
            std::snprintf(overlayBuf, sizeof(overlayBuf), "Min: %.1f ms | Max: %.1f ms", minHistoryMs, maxHistoryMs);
            ImGui::PlotLines(
                "##FrameTimeGraph",
                m_FrameTimeHistoryMs,
                kFrameHistoryCount,
                m_FrameHistoryOffset,
                overlayBuf,
                0.0f,
                std::max(33.3f, maxHistoryMs * 1.15f),
                ImVec2(overlayW - 20.0f, 34.0f)
            );

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.78f, 0.84f, 0.94f, 1.0f), "GPU Pipeline (Scene Pass)");

            const auto& rStats = engineCtx.renderSystem.scene_stats();
            const uint32_t totalTris = rStats.triangles_main + rStats.triangles_shadow;
            const uint32_t totalSubmeshes = rStats.visible_submeshes + rStats.culled_submeshes;

            auto drawStatRow = [](const char* label, const char* fmt, ...)
            {
                ImGui::TextDisabled("%s", label);
                ImGui::SameLine(125.0f);
                va_list args;
                va_start(args, fmt);
                ImGui::TextV(fmt, args);
                va_end(args);
            };

            drawStatRow("Draw Calls:", "%u (Main: %u, Shd: %u)",
                rStats.total_draw_calls, rStats.draw_calls_main, rStats.draw_calls_shadow);

            if (totalTris >= 1000)
            {
                drawStatRow("Triangles:", "%.1fk (Main: %.1fk)",
                    static_cast<float>(totalTris) / 1000.0f,
                    static_cast<float>(rStats.triangles_main) / 1000.0f);
            }
            else
            {
                drawStatRow("Triangles:", "%u (Main: %u, Shd: %u)",
                    totalTris, rStats.triangles_main, rStats.triangles_shadow);
            }

            drawStatRow("Shader Batches:", "%u active", rStats.shader_batches);
            drawStatRow("Frustum Culling:", "%u vis / %u culled (of %u)",
                rStats.visible_submeshes, rStats.culled_submeshes, totalSubmeshes);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.78f, 0.84f, 0.94f, 1.0f), "World & ECS Telemetry");

            const auto& reg = engineCtx.session.editor_scene().get_registry();
            const size_t entityCount = reg.get_all_entities().size();
            const size_t staticMeshes = reg.get_all<mesh_component>().size();
            const size_t skinnedMeshes = reg.get_all<skinned_mesh_component>().size();
            const size_t rbCount = reg.get_all<rigidbody_component>().size();

            drawStatRow("Entities:", "%zu active", entityCount);
            drawStatRow("Mesh Comps:", "%zu static / %zu skinned", staticMeshes, skinnedMeshes);
            drawStatRow("Active Lights:", "%u / %d (%s)",
                rStats.active_lights,
                static_cast<int>(MAX_LIGHT_COUNT),
                rStats.shadow_pass_active ? "Shadows ON" : "No Shadows");

            const char* simStatus = "Editor (Idle)";
            if (editorCtx.mode == EngineMode::PLAY)
            {
                simStatus = editorCtx.isPaused ? "Paused (Zero-Copy)" : "Playing (Zero-Copy)";
            }
            drawStatRow("Physics / Mode:", "%zu RBs | %s", rbCount, simStatus);
            drawStatRow("Viewport Res:", "%d x %d px",
                static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    void scene_gizmo_overlay::render_billboard_icons(
        EngineContext& engineCtx,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& camPos,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize,
        bool allowClick)
    {
        const auto& reg = engineCtx.session.editor_scene().get_registry();
        const auto& settings = engineCtx.gizmoSystem.settings();
        const glm::mat4 viewProj = proj * view;

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool leftClicked = allowClick && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const int selectedID = engineCtx.vestaSys.selection().get_first_selected_entity();

        ImDrawList* dl = ImGui::GetWindowDrawList();

        for (entity e : reg.get_all_entities())
        {
            const entity_icon_type iconType = resolve_entity_icon(e, reg, settings);
            if (iconType == entity_icon_type::none || iconType == entity_icon_type::auto_detect) {
                continue;
            }

            if (!reg.has_component<transform_component>(e)) {
                continue;
            }

            const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
            const glm::vec3 worldPos = glm::vec3(worldMat[3]);

            ImVec2 screenPt;
            float clipW = 1.0f;
            if (!gizmo_math::project_world_to_screen(worldPos, viewProj, viewportPos, viewportSize, screenPt, &clipW)) {
                continue;
            }

            const float dist = std::max(1.0f, glm::length(worldPos - camPos));
            float radius = 16.0f * settings.icon_size;
            if (settings.icon_distance_scaling)
            {
                const float distFactor = std::clamp(10.0f / (dist * 0.45f + 4.0f), 0.55f, 1.45f);
                radius *= distFactor;
            }
            radius = std::clamp(radius, 9.0f, 36.0f);

            glm::vec3 tint(1.0f);
            if (const auto* light = reg.get_component<light_component>(e))
            {
                tint = light->color;
            }

            const float distToMouse = std::hypot(mousePos.x - screenPt.x, mousePos.y - screenPt.y);
            const bool isHovered = allowClick && (distToMouse <= radius + 3.0f);
            const bool isSelected = (selectedID != -1 && static_cast<int>(e) == selectedID);

            if (isSelected || isHovered)
            {
                const ImU32 ringCol = isSelected
                    ? ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.78f, 0.18f, 0.95f))
                    : ImGui::ColorConvertFloat4ToU32(ImVec4(0.45f, 0.82f, 1.0f, 0.90f));
                dl->AddCircle(screenPt, radius + 3.0f, ringCol, 24, 2.2f);
            }

            draw_icon_badge_2d(iconType, glm::vec2(screenPt.x, screenPt.y), radius, tint);

            // Özel etiket rozetlerinde nesne adını yanında göster
            if (iconType >= entity_icon_type::label_blue && iconType <= entity_icon_type::label_purple)
            {
                if (const auto* nc = reg.get_component<name_component>(e))
                {
                    if (!nc->name.empty())
                    {
                        const ImVec2 textPos(screenPt.x + radius + 5.0f, screenPt.y - 7.0f);
                        dl->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 210), nc->name.c_str());
                        dl->AddText(textPos, IM_COL32(245, 248, 255, 245), nc->name.c_str());
                    }
                }
            }

            if (isHovered)
            {
                const auto* nc = reg.get_component<name_component>(e);
                const char* entName = (nc && !nc->name.empty()) ? nc->name.c_str() : "Entity";
                ImGui::SetTooltip("%s (%s)\nClick to select in Scene", entName, get_icon_type_name(iconType));

                if (leftClicked)
                {
                    engineCtx.vestaSys.selection().select_entity(e, ImGui::GetIO().KeyCtrl);
                }
            }
        }
    }

    void scene_gizmo_overlay::render_transform_gizmo(
        EngineContext& engineCtx,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& camPos,
        float camFovDeg,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        auto& settings = engineCtx.gizmoSystem.settings();
        if (settings.active_tool == gizmo_tool_mode::select)
        {
            m_TranslationTool.reset_interaction();
            m_RotationTool.reset_interaction();
            m_ScaleTool.reset_interaction();
            return;
        }

        auto& reg = engineCtx.session.editor_scene().get_registry();
        const auto& selectedEntities = engineCtx.vestaSys.selection().get_selected_entities();
        if (selectedEntities.empty())
        {
            m_TranslationTool.reset_interaction();
            m_RotationTool.reset_interaction();
            m_ScaleTool.reset_interaction();
            return;
        }

        // Çoklu seçimde dönüşüm bileşenine sahip tüm varlıkları topla ve merkezlerini ortala
        std::vector<entity> activeEntities;
        glm::vec3 centerWorld(0.0f);
        for (entity ent : selectedEntities)
        {
            if (reg.has_component<transform_component>(ent))
            {
                activeEntities.push_back(ent);
                const glm::mat4 wMat = ecs_transform::get_world_matrix(ent, reg);
                centerWorld += glm::vec3(wMat[3]);
            }
        }

        if (activeEntities.empty())
        {
            m_TranslationTool.reset_interaction();
            m_RotationTool.reset_interaction();
            m_ScaleTool.reset_interaction();
            return;
        }

        centerWorld /= static_cast<float>(activeEntities.size());
        const glm::vec3 originWorld = centerWorld;
        const entity primaryEntity = activeEntities.front();
        auto* primaryTransform = reg.get_component<transform_component>(primaryEntity);

        const glm::mat4 viewProj = proj * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);

        ImVec2 originScreen;
        if (!gizmo_math::project_world_to_screen(originWorld, viewProj, viewportPos, viewportSize, originScreen))
        {
            return;
        }

        const float distToCam = std::max(0.25f, glm::length(originWorld - camPos));
        const float tanHalfFov = std::tan(glm::radians(std::clamp(camFovDeg, 10.0f, 150.0f)) * 0.5f);
        const float gizmoWorldScale = (distToCam * tanHalfFov * 2.0f) * (settings.handle_size / std::max(100.0f, viewportSize.y));

        const bool useLocal = (settings.transform_space == gizmo_space::local) || (settings.active_tool == gizmo_tool_mode::scale);
        glm::mat3 basis(1.0f);
        if (useLocal && primaryTransform)
        {
            basis = gizmo_math::euler_to_mat3(primaryTransform->rotation);
        }

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool snapActive = settings.snap_enabled || ImGui::GetIO().KeyCtrl;
        ImDrawList* dl = ImGui::GetWindowDrawList();

        const gizmo_tool_context toolCtx = {
            engineCtx,
            view,
            proj,
            viewProj,
            invViewProj,
            camPos,
            camFovDeg,
            viewportPos,
            viewportSize,
            mousePos,
            mouseDown,
            mouseClicked,
            snapActive,
            gizmoWorldScale,
            originWorld,
            originScreen,
            basis,
            activeEntities,
            primaryEntity,
            dl
        };

        if (settings.active_tool == gizmo_tool_mode::translate)
        {
            m_RotationTool.reset_interaction();
            m_ScaleTool.reset_interaction();
            m_TranslationTool.process(toolCtx);
        }
        else if (settings.active_tool == gizmo_tool_mode::rotate)
        {
            m_TranslationTool.reset_interaction();
            m_ScaleTool.reset_interaction();
            m_RotationTool.process(toolCtx);
        }
        else if (settings.active_tool == gizmo_tool_mode::scale)
        {
            m_TranslationTool.reset_interaction();
            m_RotationTool.reset_interaction();
            m_ScaleTool.process(toolCtx);
        }
    }

    void scene_gizmo_overlay::render_ui_overlay(
        EngineContext& engineCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        scene& activeScene = engineCtx.session.editor_scene();
        registry& reg = activeScene.get_registry();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        bool hasCanvas = false;
        entity canvasEntity = 0;
        for (entity e : reg.get_all_entities())
        {
            if (reg.has_component<canvas_component>(e))
            {
                hasCanvas = true;
                canvasEntity = e;
                break;
            }
        }

        // 1. Canvas Dış Sınır Çizgisi (Gizmo)
        if (hasCanvas)
        {
            const ImVec2 canvasMin(viewportPos.x + 1.0f, viewportPos.y + 1.0f);
            const ImVec2 canvasMax(viewportPos.x + viewportSize.x - 1.0f, viewportPos.y + viewportSize.y - 1.0f);

            // Çerçeve (Açık Mavi / Cyan)
            dl->AddRect(canvasMin, canvasMax, IM_COL32(65, 175, 255, 180), 0.0f, 0, 1.5f);

            // Sol üst Canvas bilgi etiketi
            auto* canvasComp = canvasEntity != 0 ? reg.get_component<canvas_component>(canvasEntity) : nullptr;
            char canvasTag[128];
            if (canvasComp) {
                snprintf(canvasTag, sizeof(canvasTag), "Canvas: Screen Space (%.0fx%.0f)", canvasComp->reference_resolution.x, canvasComp->reference_resolution.y);
            } else {
                snprintf(canvasTag, sizeof(canvasTag), "Canvas: Screen Space");
            }

            const ImVec2 tagPos(viewportPos.x + 8.0f, viewportPos.y + 46.0f);
            ImVec2 tagSize = ImGui::CalcTextSize(canvasTag);
            dl->AddRectFilled(ImVec2(tagPos.x - 4.0f, tagPos.y - 2.0f), ImVec2(tagPos.x + tagSize.x + 6.0f, tagPos.y + tagSize.y + 2.0f), IM_COL32(20, 32, 48, 190), 3.0f);
            dl->AddText(tagPos, IM_COL32(140, 210, 255, 230), canvasTag);
        }

        const auto& selectedEntities = engineCtx.vestaSys.selection().get_selected_entities();

        // 2. Tüm UI Elemanlarının Çizimi (Görseller, Metinler ve Sınır Çizgileri)
        for (entity e : reg.get_all_entities())
        {
            if (!reg.has_component<rect_transform_component>(e))
            {
                continue;
            }

            // Eğer Canvas kök varlığıysa ve üzerinde ayrı bir resim/yazı yoksa atla
            if (reg.has_component<canvas_component>(e) &&
                !reg.has_component<ui_image_component>(e) &&
                !reg.has_component<ui_text_component>(e))
            {
                continue;
            }

            auto* rt = reg.get_component<rect_transform_component>(e);
            if (!rt) continue;

            // Ebeveyn (Parent) Ekran Sınırlarını Belirle (Canvas veya Ebeveyn RectTransform)
            glm::vec4 parentRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
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

            // Anchor ve Pivot Ekran Koordinatı Hesabı
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

            const bool isSelected = (selectedEntities.find(e) != selectedEntities.end());

            // A. UI Image / Box Çizimi (Doku desteğiyle)
            if (reg.has_component<ui_image_component>(e) && rt->visible)
            {
                auto* img = reg.get_component<ui_image_component>(e);
                glm::vec4 col = img->color;
                if (reg.has_component<ui_button_component>(e))
                {
                    auto* btn = reg.get_component<ui_button_component>(e);
                    col = btn->normal_color;
                }
                const ImU32 fillCol = ImColor(col.r, col.g, col.b, col.a);

                bool renderedTex = false;
                if (img->texture_handle.is_valid() && engineCtx.aria_system)
                {
                    aria::gpu_texture_id texId = engineCtx.aria_system->get_texture_id(img->texture_handle);
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

            // B. UI Text Çizimi (Font boyut ölçeklemeli)
            if (reg.has_component<ui_text_component>(e) && rt->visible)
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

            // C. UI Elemanının Sınır Çerçeve Çizgisi ve Anchor Gizmoları
            if (!isSelected)
            {
                dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(110, 165, 235, 90), 0.0f, 0, 1.0f);
            }
            else
            {
                dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(65, 175, 255, 240), 0.0f, 0, 1.8f);

                const glm::vec2 anchorPoint(anchorMinX + anchorW * 0.5f, anchorTopY + anchorH * 0.5f);
                dl->AddLine(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(pivotScreen.x, pivotScreen.y), IM_COL32(255, 230, 70, 190), 1.2f);

                const float tS = 7.0f;
                const ImU32 triFill = IM_COL32(255, 230, 70, 245);
                const ImU32 triBorder = IM_COL32(20, 22, 28, 220);

                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), triBorder, 1.0f);

                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triBorder, 1.0f);

                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), triBorder, 1.0f);

                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triBorder, 1.0f);

                dl->AddCircleFilled(ImVec2(pivotScreen.x, pivotScreen.y), 4.0f, IM_COL32(65, 185, 255, 255));
                dl->AddCircle(ImVec2(pivotScreen.x, pivotScreen.y), 4.0f, IM_COL32(20, 22, 28, 220), 0, 1.2f);
            }

            // D. Tıklayarak seçme
            const ImVec2 mousePos = ImGui::GetIO().MousePos;
            const bool isLeftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            if (isLeftClicked && !is_interacting())
            {
                if (mousePos.x >= x0 && mousePos.x <= x1 && mousePos.y >= y0 && mousePos.y <= y1)
                {
                    engineCtx.vestaSys.selection().select_entity(e);
                }
            }
        }
    }
} // namespace vesta
