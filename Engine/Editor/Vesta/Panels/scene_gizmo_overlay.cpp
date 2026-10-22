/**
 * @file scene_gizmo_overlay.cpp
 * @brief Sahne görünümü (Scene Viewport) etkileşimli Transform araçları, Işık menzil/koni tutamaçları,
 *        Billboard ikonları ve Gizmo ayar araç çubuğu uygulaması.
 */

#include "scene_gizmo_overlay.h"
#include "../../../Engine/engine_context.h"
#include "../../../Editor/editor_context.h"
#include "../../../Editor/editor_camera.h"
#include "../vesta.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdarg>

namespace vesta
{
    namespace
    {
        // ====================================================================
        // 3B PROJEKSİYON VE IŞIN KESİŞİM MATEMATİĞİ YARDIMCILARI
        // ====================================================================

        struct Ray3D
        {
            glm::vec3 origin{ 0.0f };
            glm::vec3 dir{ 0.0f, 0.0f, -1.0f };
        };

        /**
         * @brief Dünya uzayındaki bir 3B noktayı viewport ekran koordinatına izdüşürür.
         */
        bool project_world_to_screen(
            const glm::vec3& worldPos,
            const glm::mat4& viewProj,
            const glm::vec2& vpPos,
            const glm::vec2& vpSize,
            ImVec2& outScreen,
            float* outClipW = nullptr)
        {
            const glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
            if (outClipW) {
                *outClipW = clip.w;
            }
            if (clip.w <= 0.01f) {
                return false;
            }

            const glm::vec3 ndc = glm::vec3(clip) / clip.w;
            outScreen.x = vpPos.x + (ndc.x * 0.5f + 0.5f) * vpSize.x;
            outScreen.y = vpPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y;
            return true;
        }

        /**
         * @brief Ekran fare koordinatından 3B dünya ışını (Ray) üretir.
         */
        Ray3D screen_to_world_ray(
            const ImVec2& mousePos,
            const glm::mat4& invViewProj,
            const glm::vec2& vpPos,
            const glm::vec2& vpSize)
        {
            const float nx = ((mousePos.x - vpPos.x) / std::max(1.0f, vpSize.x)) * 2.0f - 1.0f;
            const float ny = 1.0f - ((mousePos.y - vpPos.y) / std::max(1.0f, vpSize.y)) * 2.0f;

            glm::vec4 nearPt = invViewProj * glm::vec4(nx, ny, -1.0f, 1.0f);
            glm::vec4 farPt  = invViewProj * glm::vec4(nx, ny,  1.0f, 1.0f);

            nearPt /= nearPt.w;
            farPt  /= farPt.w;

            Ray3D ray;
            ray.origin = glm::vec3(nearPt);
            const glm::vec3 diff = glm::vec3(farPt - nearPt);
            ray.dir = glm::length(diff) > 1e-6f ? glm::normalize(diff) : glm::vec3(0.0f, 0.0f, -1.0f);
            return ray;
        }

        /**
         * @brief Bir 2B noktanın doğru parçasına olan en kısa piksel mesafesini hesaplar.
         */
        float dist_point_to_segment_2d(const ImVec2& p, const ImVec2& a, const ImVec2& b)
        {
            const float vx = b.x - a.x;
            const float vy = b.y - a.y;
            const float wx = p.x - a.x;
            const float wy = p.y - a.y;

            const float c1 = wx * vx + wy * vy;
            if (c1 <= 0.0f) {
                return std::hypot(p.x - a.x, p.y - a.y);
            }

            const float c2 = vx * vx + vy * vy;
            if (c2 <= c1) {
                return std::hypot(p.x - b.x, p.y - b.y);
            }

            const float t = c1 / c2;
            const float projX = a.x + t * vx;
            const float projY = a.y + t * vy;
            return std::hypot(p.x - projX, p.y - projY);
        }

        /**
         * @brief Fare ışını ile 3B eksen doğrusu arasındaki en yakın noktanın eksen parametresini (t) hesaplar.
         */
        float ray_axis_closest_param(
            const Ray3D& ray,
            const glm::vec3& axisOrigin,
            const glm::vec3& axisDir)
        {
            const glm::vec3 w0 = ray.origin - axisOrigin;
            const float a = glm::dot(ray.dir, ray.dir);
            const float b = glm::dot(ray.dir, axisDir);
            const float c = glm::dot(axisDir, axisDir);
            const float d = glm::dot(ray.dir, w0);
            const float e = glm::dot(axisDir, w0);

            const float denom = a * c - b * b;
            if (std::abs(denom) < 1e-5f) {
                return 0.0f;
            }
            return (a * e - b * d) / denom;
        }

        /**
         * @brief Fare ışını ile 3B düzlemin kesişim noktasını hesaplar.
         */
        bool ray_plane_intersect(
            const Ray3D& ray,
            const glm::vec3& planeOrigin,
            const glm::vec3& planeNormal,
            glm::vec3& outHitPoint)
        {
            const float denom = glm::dot(planeNormal, ray.dir);
            if (std::abs(denom) < 1e-5f) {
                return false;
            }

            const float t = glm::dot(planeOrigin - ray.origin, planeNormal) / denom;
            if (t < 0.0f) {
                return false;
            }

            outHitPoint = ray.origin + ray.dir * t;
            return true;
        }

        /**
         * @brief Değeri verilen adıma (snapStep) yuvarlar.
         */
        float snap_value(float val, float step)
        {
            if (step <= 1e-4f) {
                return val;
            }
            return std::round(val / step) * step;
        }

        /**
         * @brief Euler açılarından (Rx * Ry * Rz) 3x3 rotasyon matrisini üretir.
         */
        glm::mat3 euler_to_mat3(const glm::vec3& rot)
        {
            glm::mat4 m(1.0f);
            m = glm::rotate(m, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
            m = glm::rotate(m, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
            m = glm::rotate(m, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
            return glm::mat3(m);
        }

        /**
         * @brief 3x3 rotasyon matrisini motorun Rx * Ry * Rz Euler açısı dizilimine ayrıştırır.
         */
        glm::vec3 mat3_to_euler(const glm::mat3& R)
        {
            float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
            const float sinYaw = glm::clamp(R[2][0], -1.0f, 1.0f);

            if (sinYaw < 0.9999f)
            {
                if (sinYaw > -0.9999f)
                {
                    pitch = std::atan2(-R[2][1], R[2][2]);
                    yaw   = std::asin(sinYaw);
                    roll  = std::atan2(-R[1][0], R[0][0]);
                }
                else
                {
                    pitch = 0.0f;
                    yaw   = -glm::half_pi<float>();
                    roll  = -std::atan2(R[0][1], R[1][1]);
                }
            }
            else
            {
                pitch = 0.0f;
                yaw   = glm::half_pi<float>();
                roll  = std::atan2(R[0][1], R[1][1]);
            }

            return glm::vec3(pitch, yaw, roll);
        }

        /**
         * @brief Verilen normal vektörüne dik iki ortonormal taban vektörü üretir.
         */
        void build_basis(const glm::vec3& normal, glm::vec3& outRight, glm::vec3& outUp)
        {
            const glm::vec3 n = glm::length(normal) > 1e-5f ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::vec3 helper = (std::abs(n.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
            outRight = glm::normalize(glm::cross(helper, n));
            outUp = glm::normalize(glm::cross(n, outRight));
        }

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

        // Arka plan dairesi
        dl->AddCircleFilled(c, r, toCol(0.11f, 0.13f, 0.17f, 0.88f), 24);
        dl->AddCircle(c, r, toCol(0.75f, 0.80f, 0.88f, 0.65f), 24, 1.3f);

        const glm::vec3 safeTint = glm::clamp(customTint, glm::vec3(0.35f), glm::vec3(1.0f));

        switch (type)
        {
        case entity_icon_type::camera:
        {
            const ImU32 camCol = toCol(0.88f, 0.94f, 1.0f, 0.98f);
            // Kamera gövdesi
            const ImVec2 bMin(c.x - r * 0.52f, c.y - r * 0.28f);
            const ImVec2 bMax(c.x + r * 0.18f, c.y + r * 0.36f);
            dl->AddRectFilled(bMin, bMax, camCol, 2.0f);
            // Kamera lens hunisi
            const ImVec2 p0(c.x + r * 0.16f, c.y + r * 0.04f);
            const ImVec2 p1(c.x + r * 0.56f, c.y - r * 0.30f);
            const ImVec2 p2(c.x + r * 0.56f, c.y + r * 0.38f);
            dl->AddTriangleFilled(p0, p1, p2, camCol);
            // Üstteki iki film makarası
            dl->AddCircleFilled(ImVec2(c.x - r * 0.34f, c.y - r * 0.42f), r * 0.17f, camCol, 12);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.01f, c.y - r * 0.42f), r * 0.17f, camCol, 12);
            break;
        }
        case entity_icon_type::directional_light:
        {
            const ImU32 sunCol = toCol(1.0f, 0.84f, 0.24f, 1.0f);
            dl->AddCircleFilled(c, r * 0.34f, sunCol, 16);
            for (int i = 0; i < 8; ++i)
            {
                const float a = (glm::two_pi<float>() / 8.0f) * static_cast<float>(i);
                const float cs = std::cos(a);
                const float sn = std::sin(a);
                dl->AddLine(
                    ImVec2(c.x + cs * r * 0.46f, c.y + sn * r * 0.46f),
                    ImVec2(c.x + cs * r * 0.72f, c.y + sn * r * 0.72f),
                    sunCol, 1.8f);
            }
            break;
        }
        case entity_icon_type::point_light:
        {
            const ImU32 bulbCol = toCol(safeTint.r, safeTint.g, safeTint.b * 0.75f + 0.25f, 1.0f);
            // Ampul küresi
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.10f), r * 0.38f, bulbCol, 16);
            // Ampul duy tabanı
            dl->AddRectFilled(
                ImVec2(c.x - r * 0.20f, c.y + r * 0.22f),
                ImVec2(c.x + r * 0.20f, c.y + r * 0.52f),
                toCol(0.80f, 0.84f, 0.90f, 0.95f), 1.5f);
            // Işıma çizgileri
            for (int i = -2; i <= 2; ++i)
            {
                const float a = -glm::half_pi<float>() + static_cast<float>(i) * 0.62f;
                dl->AddLine(
                    ImVec2(c.x + std::cos(a) * r * 0.50f, c.y - r * 0.10f + std::sin(a) * r * 0.50f),
                    ImVec2(c.x + std::cos(a) * r * 0.72f, c.y - r * 0.10f + std::sin(a) * r * 0.72f),
                    bulbCol, 1.5f);
            }
            break;
        }
        case entity_icon_type::spot_light:
        {
            const ImU32 spotCol = toCol(safeTint.r, safeTint.g, safeTint.b * 0.7f + 0.3f, 1.0f);
            const ImVec2 apex(c.x - r * 0.45f, c.y - r * 0.25f);
            const ImVec2 b1(c.x + r * 0.48f, c.y - r * 0.10f);
            const ImVec2 b2(c.x + r * 0.10f, c.y + r * 0.54f);
            dl->AddTriangleFilled(apex, b1, b2, toCol(safeTint.r, safeTint.g, safeTint.b, 0.42f));
            dl->AddTriangle(apex, b1, b2, spotCol, 1.8f);
            dl->AddCircleFilled(apex, r * 0.20f, spotCol, 12);
            break;
        }
        case entity_icon_type::spawn_point:
        {
            const ImU32 flagCol = toCol(0.24f, 0.92f, 0.54f, 1.0f);
            dl->AddLine(ImVec2(c.x - r * 0.32f, c.y + r * 0.55f), ImVec2(c.x - r * 0.32f, c.y - r * 0.55f), toCol(0.9f, 0.9f, 0.95f, 1.0f), 2.0f);
            dl->AddTriangleFilled(
                ImVec2(c.x - r * 0.30f, c.y - r * 0.55f),
                ImVec2(c.x + r * 0.52f, c.y - r * 0.22f),
                ImVec2(c.x - r * 0.30f, c.y + r * 0.10f),
                flagCol);
            break;
        }
        case entity_icon_type::trigger_zone:
        {
            const ImU32 trigCol = toCol(0.22f, 0.88f, 0.96f, 1.0f);
            const ImVec2 pts[4] = {
                ImVec2(c.x, c.y - r * 0.58f),
                ImVec2(c.x + r * 0.58f, c.y),
                ImVec2(c.x, c.y + r * 0.58f),
                ImVec2(c.x - r * 0.58f, c.y)
            };
            dl->AddConvexPolyFilled(pts, 4, toCol(0.22f, 0.88f, 0.96f, 0.35f));
            dl->AddPolyline(pts, 4, trigCol, ImDrawFlags_Closed, 2.0f);
            break;
        }
        case entity_icon_type::audio_source:
        {
            const ImU32 audCol = toCol(0.98f, 0.62f, 0.24f, 1.0f);
            dl->AddRectFilled(ImVec2(c.x - r * 0.50f, c.y - r * 0.22f), ImVec2(c.x - r * 0.22f, c.y + r * 0.22f), audCol);
            dl->AddTriangleFilled(
                ImVec2(c.x - r * 0.25f, c.y),
                ImVec2(c.x + r * 0.12f, c.y - r * 0.45f),
                ImVec2(c.x + r * 0.12f, c.y + r * 0.45f),
                audCol);
            dl->AddBezierQuadratic(
                ImVec2(c.x + r * 0.26f, c.y - r * 0.32f),
                ImVec2(c.x + r * 0.56f, c.y),
                ImVec2(c.x + r * 0.26f, c.y + r * 0.32f),
                audCol, 1.8f);
            break;
        }
        case entity_icon_type::waypoint:
        {
            const ImU32 wpCol = toCol(0.96f, 0.32f, 0.46f, 1.0f);
            dl->AddCircle(c, r * 0.45f, wpCol, 16, 1.8f);
            dl->AddCircleFilled(c, r * 0.16f, wpCol, 12);
            dl->AddLine(ImVec2(c.x - r * 0.68f, c.y), ImVec2(c.x + r * 0.68f, c.y), wpCol, 1.5f);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.68f), ImVec2(c.x, c.y + r * 0.68f), wpCol, 1.5f);
            break;
        }
        default:
        {
            // Renkli Etiket Rozetleri (Label Blue / Green / Yellow / Orange / Red / Purple)
            ImU32 labelCol = toCol(0.30f, 0.65f, 1.0f, 1.0f);
            if (type == entity_icon_type::label_green)  labelCol = toCol(0.24f, 0.86f, 0.42f, 1.0f);
            if (type == entity_icon_type::label_yellow) labelCol = toCol(0.98f, 0.84f, 0.20f, 1.0f);
            if (type == entity_icon_type::label_orange) labelCol = toCol(0.98f, 0.54f, 0.16f, 1.0f);
            if (type == entity_icon_type::label_red)    labelCol = toCol(0.94f, 0.26f, 0.28f, 1.0f);
            if (type == entity_icon_type::label_purple) labelCol = toCol(0.72f, 0.36f, 0.96f, 1.0f);

            dl->AddCircleFilled(c, r * 0.56f, labelCol, 16);
            dl->AddCircle(c, r * 0.56f, toCol(1.0f, 1.0f, 1.0f, 0.85f), 16, 1.5f);
            break;
        }
        }
    }

    // ========================================================================
    // ANA RENDER GİRİŞ NOKTASI
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

        // 1. Kısayol Tuşlarını İşle (Q, W, E, R, T, X, G, 2)
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
                render_interactive_light_handles(engineCtx, view, proj, camPos, viewportPos, viewportSize);
            }

            // 2b. Etkileşimli Ses Menzil Tutamaçları (Audio Source Min/Max Distance)
            if (settings.interactive_audio_handles && settings.audio_gizmo_mode != gizmo_visibility_mode::off)
            {
                render_interactive_audio_handles(engineCtx, view, proj, camPos, viewportPos, viewportSize);
            }

            // 3. UI Katmanı (Canvas Sınırları, Metinler, Görseller ve UI Sınır Çizgileri)
            render_ui_overlay(engineCtx, viewportPos, viewportSize);

            // 4. Etkileşimli 3B Dönüşüm Gizmosu veya 2B UI Rect Tool
            if (settings.active_tool == gizmo_tool_mode::rect_tool)
            {
                render_rect_tool(engineCtx, viewportPos, viewportSize);
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

    // ========================================================================
    // 1. KISAYOL TUŞLARI (Q / W / E / R / T / X / G / 2)
    // ========================================================================

    void scene_gizmo_overlay::process_shortcuts(
        EngineContext& engineCtx,
        EditorContext& editorCtx,
        bool isViewportFocused,
        bool isViewportHovered)
    {
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

    // ========================================================================
    // 2. SAHNE ÜST ARAÇ ÇUBUĞU VE GİZMOS POPUP MENÜSÜ
    // ========================================================================

    void scene_gizmo_overlay::render_toolbar(
        EngineContext& engineCtx,
        EditorContext& editorCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
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

    // ========================================================================
    // 2.5. PERFORMANS VE RENDER İSTATİSTİKLERİ (STATS OVERLAY)
    // ========================================================================

    void scene_gizmo_overlay::render_stats_overlay(
        EngineContext& engineCtx,
        EditorContext& editorCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        auto& settings = engineCtx.gizmoSystem.settings();
        const ImGuiIO& io = ImGui::GetIO();

        // 1. Anlık ve Yumuşatılmış (EMA) FPS / Kare Süresi Hesaplaması
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

        // 2. Pencere Konumu (Sahne Sağ-Üst Köşesi, Araç Çubuğunun Hemen Altı)
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
            // Başlık ve Kapatma [X] Butonu
            ImGui::TextColored(ImVec4(0.45f, 0.80f, 1.0f, 1.0f), "Scene & Render Stats");
            ImGui::SameLine(overlayW - 28.0f);
            if (ImGui::SmallButton("x"))
            {
                settings.show_stats_overlay = false;
            }
            ImGui::Separator();

            // FPS Renk Kodlaması (Yeşil >= 55, Sarı >= 30, Kırmızı < 30)
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

            // Canlı Kare Süresi (Frame Time) Sparkline Grafiği
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

    // ========================================================================
    // 3. BİLLBOARD İKONLAR (GÖRÜNMEZ NESNELER VE TIKLAYARAK SEÇİM)
    // ========================================================================

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
            if (!project_world_to_screen(worldPos, viewProj, viewportPos, viewportSize, screenPt, &clipW)) {
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

    // ========================================================================
    // 4. UNITY TARZI ETKİLEŞİMLİ IŞIK MENZİL VE KONİ TUTAMAÇLARI
    // ========================================================================

    void scene_gizmo_overlay::render_interactive_light_handles(
        EngineContext& engineCtx,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& camPos,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        auto& reg = engineCtx.session.editor_scene().get_registry();
        const int selectedID = engineCtx.vestaSys.selection().get_first_selected_entity();
        if (selectedID == -1)
        {
            m_ActiveLightHandle = -1;
            m_HoveredLightHandle = -1;
            return;
        }

        const entity e = static_cast<entity>(selectedID);
        auto* light = reg.get_component<light_component>(e);
        const auto* transform = reg.get_component<transform_component>(e);
        if (!light || !transform || light->type == light_type::directional)
        {
            m_ActiveLightHandle = -1;
            m_HoveredLightHandle = -1;
            return;
        }

        const glm::mat4 viewProj = proj * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);
        const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
        const glm::vec3 worldPos = glm::vec3(worldMat[3]);

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (!mouseDown)
        {
            m_ActiveLightHandle = -1;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const Ray3D mouseRay = screen_to_world_ray(mousePos, invViewProj, viewportPos, viewportSize);

        struct LightHandlePoint
        {
            int id;
            glm::vec3 worldPt;
            glm::vec3 axisDir;
            bool isInnerCone;
        };

        std::vector<LightHandlePoint> handles;

        // Işık yönü ve ortonormal tabanı
        const glm::mat3 rotMat = euler_to_mat3(transform->rotation);
        const glm::vec3 forward = glm::normalize(rotMat * glm::vec3(0.0f, 0.0f, -1.0f));
        glm::vec3 right, up;
        build_basis(forward, right, up);

        if (light->type == light_type::point)
        {
            const float r = std::max(0.1f, light->range);
            const glm::vec3 dirs[6] = {
                glm::vec3( 1.0f,  0.0f,  0.0f),
                glm::vec3(-1.0f,  0.0f,  0.0f),
                glm::vec3( 0.0f,  1.0f,  0.0f),
                glm::vec3( 0.0f, -1.0f,  0.0f),
                glm::vec3( 0.0f,  0.0f,  1.0f),
                glm::vec3( 0.0f,  0.0f, -1.0f)
            };
            for (int i = 0; i < 6; ++i)
            {
                handles.push_back({ i, worldPos + dirs[i] * r, dirs[i], false });
            }
        }
        else if (light->type == light_type::spot)
        {
            const float r = std::max(0.1f, light->range);
            const float outerDeg = std::clamp(light->outer_cone, 1.0f, 85.0f);
            const float innerDeg = std::clamp(light->inner_cone, 0.5f, outerDeg);

            const float outerRad = glm::radians(outerDeg);
            const float innerRad = glm::radians(innerDeg);

            const glm::vec3 outerCenter = worldPos + forward * (r * std::cos(outerRad));
            const float outerRadius = r * std::sin(outerRad);

            const glm::vec3 innerCenter = worldPos + forward * (r * std::cos(innerRad));
            const float innerRadius = r * std::sin(innerRad);

            // Handle 0: Menzil ucu (Range)
            handles.push_back({ 0, worldPos + forward * r, forward, false });

            // Handles 1..4: Dış koni çemberi (Outer Cone Angle)
            handles.push_back({ 1, outerCenter + right * outerRadius,  right, false });
            handles.push_back({ 2, outerCenter - right * outerRadius, -right, false });
            handles.push_back({ 3, outerCenter + up * outerRadius,     up,    false });
            handles.push_back({ 4, outerCenter - up * outerRadius,    -up,    false });

            // Handles 5..8: İç koni çemberi (Inner Cone Angle)
            handles.push_back({ 5, innerCenter + right * innerRadius,  right, true });
            handles.push_back({ 6, innerCenter - right * innerRadius, -right, true });
            handles.push_back({ 7, innerCenter + up * innerRadius,     up,    true });
            handles.push_back({ 8, innerCenter - up * innerRadius,    -up,    true });
        }

        // Hover tespiti
        if (m_ActiveLightHandle == -1 && m_ActiveHandle == HandleID::None)
        {
            m_HoveredLightHandle = -1;
            float bestDist = 11.0f;
            for (const auto& h : handles)
            {
                ImVec2 sp;
                if (project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                {
                    const float d = std::hypot(mousePos.x - sp.x, mousePos.y - sp.y);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        m_HoveredLightHandle = h.id;
                    }
                }
            }

            if (m_HoveredLightHandle != -1 && mouseClicked)
            {
                m_ActiveLightHandle = m_HoveredLightHandle;
                m_LightDragStartRange = light->range;
                m_LightDragStartOuterCone = light->outer_cone;
                m_LightDragStartInnerCone = light->inner_cone;
            }
        }

        // Aktif sürükleme güncellemesi
        if (m_ActiveLightHandle != -1 && mouseDown)
        {
            for (const auto& h : handles)
            {
                if (h.id != m_ActiveLightHandle) {
                    continue;
                }

                if (light->type == light_type::point || h.id == 0)
                {
                    // Işık merkezinden h.axisDir boyunca en yakın mesafeyi bul
                    const float param = ray_axis_closest_param(mouseRay, worldPos, h.axisDir);
                    light->range = std::max(0.2f, std::abs(param));

                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Range: %.2f m", light->range);
                    ImGui::SetTooltip("%s", buf);
                }
                else if (light->type == light_type::spot)
                {
                    // Koni taban merkezinden radyal eksen boyunca uzaklığı ölç
                    const float currentAngleDeg = h.isInnerCone ? light->inner_cone : light->outer_cone;
                    const glm::vec3 discCenter = worldPos + forward * (light->range * std::cos(glm::radians(currentAngleDeg)));
                    const float radialDist = std::max(0.02f, std::abs(ray_axis_closest_param(mouseRay, discCenter, h.axisDir)));
                    const float newAngleDeg = glm::degrees(std::atan2(radialDist, std::max(0.2f, light->range)));

                    if (h.isInnerCone)
                    {
                        light->inner_cone = std::clamp(newAngleDeg, 0.5f, light->outer_cone);
                        ImGui::SetTooltip("Inner Cone: %.1f deg", light->inner_cone);
                    }
                    else
                    {
                        light->outer_cone = std::clamp(newAngleDeg, 1.0f, 85.0f);
                        if (light->inner_cone > light->outer_cone) {
                            light->inner_cone = light->outer_cone;
                        }
                        ImGui::SetTooltip("Outer Cone: %.1f deg", light->outer_cone);
                    }
                }
                break;
            }

            // Canlı ışık UBO güncellemesi için sahne ışıklarını yenile
            engineCtx.session.editor_scene().update_light_batch();
        }

        // Tutamaç noktalarını çiz (Unity tarzı sarı kare/elmas tutamaçlar)
        for (const auto& h : handles)
        {
            ImVec2 sp;
            if (!project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp)) {
                continue;
            }

            const bool isHot = (h.id == m_HoveredLightHandle) || (h.id == m_ActiveLightHandle);
            const float halfSz = isHot ? 5.5f : 3.8f;

            const ImU32 fillCol = isHot
                ? IM_COL32(255, 255, 255, 255)
                : (h.isInnerCone ? IM_COL32(120, 230, 255, 235) : IM_COL32(255, 225, 65, 240));
            const ImU32 borderCol = IM_COL32(20, 22, 28, 240);

            dl->AddRectFilled(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), fillCol, 1.0f);
            dl->AddRect(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), borderCol, 1.0f, 0, 1.4f);
        }
    }

    // ========================================================================
    void scene_gizmo_overlay::render_interactive_audio_handles(
        EngineContext& engineCtx,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& camPos,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        auto& reg = engineCtx.session.editor_scene().get_registry();
        const int selectedID = engineCtx.vestaSys.selection().get_first_selected_entity();
        if (selectedID == -1)
        {
            m_ActiveAudioHandle = -1;
            m_HoveredAudioHandle = -1;
            return;
        }

        const entity e = static_cast<entity>(selectedID);
        auto* src = reg.get_component<audio_source_component>(e);
        const auto* transform = reg.get_component<transform_component>(e);
        if (!src || !transform || !src->is_3d)
        {
            m_ActiveAudioHandle = -1;
            m_HoveredAudioHandle = -1;
            return;
        }

        const glm::mat4 viewProj = proj * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);
        const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
        const glm::vec3 worldPos = glm::vec3(worldMat[3]);

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (!mouseDown)
        {
            m_ActiveAudioHandle = -1;
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const Ray3D mouseRay = screen_to_world_ray(mousePos, invViewProj, viewportPos, viewportSize);

        struct AudioHandlePoint
        {
            int id;
            glm::vec3 worldPt;
            glm::vec3 axisDir;
            bool isMaxDistance; // false: min_distance, true: max_distance
        };

        std::vector<AudioHandlePoint> handles;

        const float minR = std::max(0.1f, src->min_distance);
        const float maxR = std::max(minR + 0.1f, src->max_distance);

        const glm::vec3 dirs[6] = {
            glm::vec3( 1.0f,  0.0f,  0.0f),
            glm::vec3(-1.0f,  0.0f,  0.0f),
            glm::vec3( 0.0f,  1.0f,  0.0f),
            glm::vec3( 0.0f, -1.0f,  0.0f),
            glm::vec3( 0.0f,  0.0f,  1.0f),
            glm::vec3( 0.0f,  0.0f, -1.0f)
        };

        // Handles 0..5: Min distance points
        for (int i = 0; i < 6; ++i)
        {
            handles.push_back({ i, worldPos + dirs[i] * minR, dirs[i], false });
        }
        // Handles 6..11: Max distance points
        for (int i = 0; i < 6; ++i)
        {
            handles.push_back({ 6 + i, worldPos + dirs[i] * maxR, dirs[i], true });
        }

        // Hover tespiti
        if (m_ActiveAudioHandle == -1 && m_ActiveHandle == HandleID::None && m_ActiveLightHandle == -1)
        {
            m_HoveredAudioHandle = -1;
            float bestDist = 11.0f;
            for (const auto& h : handles)
            {
                ImVec2 sp;
                if (project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                {
                    const float d = std::hypot(mousePos.x - sp.x, mousePos.y - sp.y);
                    if (d < bestDist)
                    {
                        bestDist = d;
                        m_HoveredAudioHandle = h.id;
                    }
                }
            }

            if (m_HoveredAudioHandle != -1 && mouseClicked)
            {
                m_ActiveAudioHandle = m_HoveredAudioHandle;
                m_AudioDragStartMinDist = src->min_distance;
                m_AudioDragStartMaxDist = src->max_distance;
            }
        }

        // Aktif sürükleme güncellemesi
        if (m_ActiveAudioHandle != -1 && mouseDown)
        {
            for (const auto& h : handles)
            {
                if (h.id != m_ActiveAudioHandle) continue;

                const float param = ray_axis_closest_param(mouseRay, worldPos, h.axisDir);
                const float distVal = std::max(0.1f, std::abs(param));

                if (!h.isMaxDistance)
                {
                    src->min_distance = std::clamp(distVal, 0.1f, src->max_distance - 0.05f);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Audio Min Distance: %.2f m", src->min_distance);
                    ImGui::SetTooltip("%s", buf);
                }
                else
                {
                    src->max_distance = std::max(src->min_distance + 0.05f, distVal);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Audio Max Distance: %.2f m", src->max_distance);
                    ImGui::SetTooltip("%s", buf);
                }
                break;
            }
        }

        // Tutamaç çizimi
        for (const auto& h : handles)
        {
            ImVec2 sp;
            if (!project_world_to_screen(h.worldPt, viewProj, viewportPos, viewportSize, sp))
                continue;

            const bool isHot = (m_HoveredAudioHandle == h.id) || (m_ActiveAudioHandle == h.id);
            const float halfSz = isHot ? 5.5f : 3.8f;

            const ImU32 fillCol = isHot
                ? IM_COL32(255, 255, 255, 255)
                : (h.isMaxDistance ? IM_COL32(255, 165, 45, 240) : IM_COL32(50, 235, 165, 240));
            const ImU32 borderCol = IM_COL32(20, 22, 28, 240);

            dl->AddRectFilled(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), fillCol, 1.0f);
            dl->AddRect(ImVec2(sp.x - halfSz, sp.y - halfSz), ImVec2(sp.x + halfSz, sp.y + halfSz), borderCol, 1.0f, 0, 1.4f);
        }
    }

    // 5. ETKİLEŞİMLİ 3B DÖNÜŞÜM GİZMOSU (TRANSLATE / ROTATE / SCALE)
    // ========================================================================

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
            m_ActiveHandle = HandleID::None;
            m_HoveredHandle = HandleID::None;
            return;
        }

        auto& reg = engineCtx.session.editor_scene().get_registry();
        const auto& selectedEntities = engineCtx.vestaSys.selection().get_selected_entities();
        if (selectedEntities.empty())
        {
            m_ActiveHandle = HandleID::None;
            m_HoveredHandle = HandleID::None;
            m_DragEntities.clear();
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
            m_ActiveHandle = HandleID::None;
            m_HoveredHandle = HandleID::None;
            m_DragEntities.clear();
            return;
        }

        centerWorld /= static_cast<float>(activeEntities.size());
        const glm::vec3 originWorld = centerWorld;
        const entity primaryEntity = activeEntities.front();
        auto* primaryTransform = reg.get_component<transform_component>(primaryEntity);

        const glm::mat4 viewProj = proj * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);

        ImVec2 originScreen;
        if (!project_world_to_screen(originWorld, viewProj, viewportPos, viewportSize, originScreen))
        {
            return;
        }

        // Kamera mesafesine göre sabit ekran boyutlu 3B gizmo ölçeği
        const float distToCam = std::max(0.25f, glm::length(originWorld - camPos));
        const float tanHalfFov = std::tan(glm::radians(std::clamp(camFovDeg, 10.0f, 150.0f)) * 0.5f);
        const float gizmoWorldScale = (distToCam * tanHalfFov * 2.0f) * (settings.handle_size / std::max(100.0f, viewportSize.y));

        // Eksen yönelimleri (Local vs World; Scale her zaman yerel eksenlerde çalışır)
        const bool useLocal = (settings.transform_space == gizmo_space::local) || (settings.active_tool == gizmo_tool_mode::scale);
        glm::mat3 basis(1.0f);
        if (useLocal && primaryTransform)
        {
            basis = euler_to_mat3(primaryTransform->rotation);
        }

        const glm::vec3 axes[3] = {
            glm::normalize(basis[0]), // X (Kırmızı)
            glm::normalize(basis[1]), // Y (Yeşil)
            glm::normalize(basis[2])  // Z (Mavi)
        };

        const ImU32 axisColors[3] = {
            IM_COL32(238, 62, 66, 255),   // X - Red
            IM_COL32(72, 226, 88, 255),   // Y - Green
            IM_COL32(64, 138, 250, 255)   // Z - Blue
        };
        const ImU32 hoverColor = IM_COL32(255, 232, 68, 255);

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool snapActive = settings.snap_enabled || ImGui::GetIO().KeyCtrl;

        if (!mouseDown)
        {
            m_ActiveHandle = HandleID::None;
            m_DragEntities.clear();
        }

        const Ray3D mouseRay = screen_to_world_ray(mousePos, invViewProj, viewportPos, viewportSize);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        auto initDragStart = [&]()
        {
            m_DragEntities.clear();
            m_DragStartLocalPositions.clear();
            m_DragStartWorldPositions.clear();
            m_DragStartRotations.clear();
            m_DragStartScales.clear();

            for (entity ent : activeEntities)
            {
                auto* t = reg.get_component<transform_component>(ent);
                if (t)
                {
                    m_DragEntities.push_back(ent);
                    m_DragStartLocalPositions.push_back(t->position);
                    m_DragStartRotations.push_back(t->rotation);
                    m_DragStartScales.push_back(t->scale);
                    const glm::mat4 wMat = ecs_transform::get_world_matrix(ent, reg);
                    m_DragStartWorldPositions.push_back(glm::vec3(wMat[3]));
                }
            }

            m_DragEntity = primaryEntity;
            m_DragStartPosition = primaryTransform ? primaryTransform->position : glm::vec3(0.0f);
            m_DragStartScale = primaryTransform ? primaryTransform->scale : glm::vec3(1.0f);
            m_DragStartRotation = primaryTransform ? primaryTransform->rotation : glm::vec3(0.0f);
            m_DragStartWorldPos = originWorld;
            m_DragStartMousePos = glm::vec2(mousePos.x, mousePos.y);
        };

        // ====================================================================
        // MOD A: TRANSLATE (KONUM TAŞIMA - W)
        // ====================================================================
        if (settings.active_tool == gizmo_tool_mode::translate)
        {
            const glm::vec3 tipWorld[3] = {
                originWorld + axes[0] * gizmoWorldScale,
                originWorld + axes[1] * gizmoWorldScale,
                originWorld + axes[2] * gizmoWorldScale
            };

            ImVec2 tipScreen[3];
            bool tipValid[3];
            for (int i = 0; i < 3; ++i)
            {
                tipValid[i] = project_world_to_screen(tipWorld[i], viewProj, viewportPos, viewportSize, tipScreen[i]);
            }

            // 1. Hover Tespiti
            if (m_ActiveHandle == HandleID::None && m_ActiveLightHandle == -1)
            {
                m_HoveredHandle = HandleID::None;
                float bestDist = 10.0f;

                // Merkez serbest taşıma dairesi
                const float centerDist = std::hypot(mousePos.x - originScreen.x, mousePos.y - originScreen.y);
                if (centerDist <= 9.5f)
                {
                    m_HoveredHandle = HandleID::Center;
                    bestDist = centerDist;
                }

                // İki eksenli düzlem kutucukları (PlaneXY, PlaneXZ, PlaneYZ)
                const int planePairs[3][2] = { {0, 1}, {0, 2}, {1, 2} };
                const HandleID planeIDs[3] = { HandleID::PlaneXY, HandleID::PlaneXZ, HandleID::PlaneYZ };
                for (int p = 0; p < 3 && m_HoveredHandle == HandleID::None; ++p)
                {
                    const glm::vec3 pCenter = originWorld + (axes[planePairs[p][0]] + axes[planePairs[p][1]]) * (gizmoWorldScale * 0.32f);
                    ImVec2 pScreen;
                    if (project_world_to_screen(pCenter, viewProj, viewportPos, viewportSize, pScreen))
                    {
                        if (std::hypot(mousePos.x - pScreen.x, mousePos.y - pScreen.y) <= 10.0f)
                        {
                            m_HoveredHandle = planeIDs[p];
                        }
                    }
                }

                // Tekil eksen okları (AxisX, AxisY, AxisZ)
                if (m_HoveredHandle == HandleID::None)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        if (!tipValid[i]) continue;
                        const float d = dist_point_to_segment_2d(mousePos, originScreen, tipScreen[i]);
                        if (d < bestDist)
                        {
                            bestDist = d;
                            m_HoveredHandle = static_cast<HandleID>(static_cast<int>(HandleID::AxisX) + i);
                        }
                    }
                }

                // Tıklama ile sürüklemeyi başlat
                if (m_HoveredHandle != HandleID::None && mouseClicked)
                {
                    m_ActiveHandle = m_HoveredHandle;
                    initDragStart();

                    if (m_ActiveHandle == HandleID::AxisX || m_ActiveHandle == HandleID::AxisY || m_ActiveHandle == HandleID::AxisZ)
                    {
                        const int idx = static_cast<int>(m_ActiveHandle) - static_cast<int>(HandleID::AxisX);
                        m_DragAxis0 = axes[idx];
                        m_DragStartAxisParam = ray_axis_closest_param(mouseRay, originWorld, m_DragAxis0);
                    }
                    else
                    {
                        if (m_ActiveHandle == HandleID::PlaneXY)      m_DragPlaneNormal = axes[2];
                        else if (m_ActiveHandle == HandleID::PlaneXZ) m_DragPlaneNormal = axes[1];
                        else if (m_ActiveHandle == HandleID::PlaneYZ) m_DragPlaneNormal = axes[0];
                        else                                          m_DragPlaneNormal = glm::normalize(camPos - originWorld);

                        if (!ray_plane_intersect(mouseRay, originWorld, m_DragPlaneNormal, m_DragStartHitPoint))
                        {
                            m_DragStartHitPoint = originWorld;
                        }
                    }
                }
            }

            // 2. Aktif Sürükleme Hesaplaması
            if (m_ActiveHandle != HandleID::None && mouseDown && !m_DragEntities.empty())
            {
                glm::vec3 newWorldPos = m_DragStartWorldPos;

                if (m_ActiveHandle == HandleID::AxisX || m_ActiveHandle == HandleID::AxisY || m_ActiveHandle == HandleID::AxisZ)
                {
                    const float curParam = ray_axis_closest_param(mouseRay, m_DragStartWorldPos, m_DragAxis0);
                    float delta = curParam - m_DragStartAxisParam;
                    if (snapActive) {
                        delta = snap_value(delta, settings.translate_snap);
                    }
                    newWorldPos = m_DragStartWorldPos + m_DragAxis0 * delta;

                    // Sonsuz eksen kılavuz çizgisi çiz
                    ImVec2 g0, g1;
                    if (project_world_to_screen(m_DragStartWorldPos - m_DragAxis0 * 50.0f, viewProj, viewportPos, viewportSize, g0) &&
                        project_world_to_screen(m_DragStartWorldPos + m_DragAxis0 * 50.0f, viewProj, viewportPos, viewportSize, g1))
                    {
                        dl->AddLine(g0, g1, IM_COL32(255, 240, 120, 110), 1.5f);
                    }
                }
                else
                {
                    glm::vec3 hitPt;
                    if (ray_plane_intersect(mouseRay, m_DragStartWorldPos, m_DragPlaneNormal, hitPt))
                    {
                        glm::vec3 deltaVec = hitPt - m_DragStartHitPoint;
                        if (snapActive)
                        {
                            deltaVec.x = snap_value(deltaVec.x, settings.translate_snap);
                            deltaVec.y = snap_value(deltaVec.y, settings.translate_snap);
                            deltaVec.z = snap_value(deltaVec.z, settings.translate_snap);
                        }
                        newWorldPos = m_DragStartWorldPos + deltaVec;
                    }
                }

                const glm::vec3 deltaWorld = newWorldPos - m_DragStartWorldPos;

                for (size_t i = 0; i < m_DragEntities.size(); ++i)
                {
                    entity ent = m_DragEntities[i];
                    auto* t = reg.get_component<transform_component>(ent);
                    if (!t) continue;

                    const glm::vec3 targetWorld = m_DragStartWorldPositions[i] + deltaWorld;

                    glm::mat4 invParent(1.0f);
                    if (const auto* hc = reg.get_component<hierarchy_component>(ent))
                    {
                        if (hc->parent != 0)
                        {
                            invParent = glm::inverse(ecs_transform::get_world_matrix(hc->parent, reg));
                        }
                    }

                    t->position = glm::vec3(invParent * glm::vec4(targetWorld, 1.0f));
                }

                if (m_DragEntities.size() > 1)
                {
                    ImGui::SetTooltip("Moving %zu Entities\nDelta: (%.2f, %.2f, %.2f)",
                        m_DragEntities.size(), deltaWorld.x, deltaWorld.y, deltaWorld.z);
                }
                else if (primaryTransform)
                {
                    ImGui::SetTooltip("Pos: (%.2f, %.2f, %.2f)",
                        primaryTransform->position.x, primaryTransform->position.y, primaryTransform->position.z);
                }
            }

            // 3. Çizim: Düzlem Kutucukları (Plane Quads)
            const int planePairs[3][2] = { {0, 1}, {0, 2}, {1, 2} };
            const HandleID planeIDs[3] = { HandleID::PlaneXY, HandleID::PlaneXZ, HandleID::PlaneYZ };
            const ImU32 planeFillCols[3] = {
                IM_COL32(64, 138, 250, 75),
                IM_COL32(72, 226, 88, 75),
                IM_COL32(238, 62, 66, 75)
            };
            const ImU32 planeBorderCols[3] = {
                IM_COL32(64, 138, 250, 210),
                IM_COL32(72, 226, 88, 210),
                IM_COL32(238, 62, 66, 210)
            };

            for (int p = 0; p < 3; ++p)
            {
                const glm::vec3 a0 = axes[planePairs[p][0]];
                const glm::vec3 a1 = axes[planePairs[p][1]];
                const float s0 = gizmoWorldScale * 0.20f;
                const float s1 = gizmoWorldScale * 0.42f;

                ImVec2 q[4];
                if (project_world_to_screen(originWorld + a0 * s0 + a1 * s0, viewProj, viewportPos, viewportSize, q[0]) &&
                    project_world_to_screen(originWorld + a0 * s1 + a1 * s0, viewProj, viewportPos, viewportSize, q[1]) &&
                    project_world_to_screen(originWorld + a0 * s1 + a1 * s1, viewProj, viewportPos, viewportSize, q[2]) &&
                    project_world_to_screen(originWorld + a0 * s0 + a1 * s1, viewProj, viewportPos, viewportSize, q[3]))
                {
                    const bool hot = (m_HoveredHandle == planeIDs[p]) || (m_ActiveHandle == planeIDs[p]);
                    dl->AddQuadFilled(q[0], q[1], q[2], q[3], hot ? IM_COL32(255, 232, 68, 140) : planeFillCols[p]);
                    dl->AddQuad(q[0], q[1], q[2], q[3], hot ? hoverColor : planeBorderCols[p], 1.6f);
                }
            }

            // 4. Çizim: X, Y, Z Eksen Okları
            for (int i = 0; i < 3; ++i)
            {
                if (!tipValid[i]) continue;
                const HandleID id = static_cast<HandleID>(static_cast<int>(HandleID::AxisX) + i);
                const bool hot = (m_HoveredHandle == id) || (m_ActiveHandle == id);
                const ImU32 col = hot ? hoverColor : axisColors[i];

                dl->AddLine(originScreen, tipScreen[i], col, hot ? 3.8f : 2.8f);

                // Ok başı üçgeni
                const float dx = tipScreen[i].x - originScreen.x;
                const float dy = tipScreen[i].y - originScreen.y;
                const float len = std::hypot(dx, dy);
                if (len > 8.0f)
                {
                    const float ux = dx / len;
                    const float uy = dy / len;
                    const float px = -uy;
                    const float py = ux;

                    const ImVec2 headTip(tipScreen[i].x + ux * 6.0f, tipScreen[i].y + uy * 6.0f);
                    const ImVec2 baseL(tipScreen[i].x - ux * 8.0f + px * 5.5f, tipScreen[i].y - uy * 8.0f + py * 5.5f);
                    const ImVec2 baseR(tipScreen[i].x - ux * 8.0f - px * 5.5f, tipScreen[i].y - uy * 8.0f - py * 5.5f);
                    dl->AddTriangleFilled(headTip, baseL, baseR, col);
                }
            }

            // 5. Çizim: Merkez Serbest Taşıma Noktası
            const bool centerHot = (m_HoveredHandle == HandleID::Center) || (m_ActiveHandle == HandleID::Center);
            dl->AddCircleFilled(originScreen, centerHot ? 6.5f : 5.0f, centerHot ? hoverColor : IM_COL32(245, 245, 250, 240), 16);
            dl->AddCircle(originScreen, centerHot ? 6.5f : 5.0f, IM_COL32(20, 22, 28, 230), 16, 1.5f);
        }

        // ====================================================================
        // MOD B: ROTATE (DÖNDÜRME - E)
        // ====================================================================
        else if (settings.active_tool == gizmo_tool_mode::rotate)
        {
            const float ringRadius = gizmoWorldScale * 0.92f;
            constexpr int ringSegments = 56;
            const glm::vec3 viewDirToCenter = glm::normalize(originWorld - camPos);

            // 1. Hover Tespiti
            if (m_ActiveHandle == HandleID::None && m_ActiveLightHandle == -1)
            {
                m_HoveredHandle = HandleID::None;
                float bestDist = 9.0f;

                for (int i = 0; i < 3; ++i)
                {
                    glm::vec3 u, v;
                    build_basis(axes[i], u, v);
                    const float step = glm::two_pi<float>() / static_cast<float>(ringSegments);

                    for (int s = 0; s < ringSegments; ++s)
                    {
                        const float a0 = static_cast<float>(s) * step;
                        const float a1 = static_cast<float>(s + 1) * step;

                        const glm::vec3 p0 = originWorld + (u * std::cos(a0) + v * std::sin(a0)) * ringRadius;
                        const glm::vec3 p1 = originWorld + (u * std::cos(a1) + v * std::sin(a1)) * ringRadius;

                        ImVec2 sp0, sp1;
                        if (project_world_to_screen(p0, viewProj, viewportPos, viewportSize, sp0) &&
                            project_world_to_screen(p1, viewProj, viewportPos, viewportSize, sp1))
                        {
                            const float d = dist_point_to_segment_2d(mousePos, sp0, sp1);
                            if (d < bestDist)
                            {
                                bestDist = d;
                                m_HoveredHandle = static_cast<HandleID>(static_cast<int>(HandleID::AxisX) + i);
                            }
                        }
                    }
                }

                if (m_HoveredHandle != HandleID::None && mouseClicked)
                {
                    m_ActiveHandle = m_HoveredHandle;
                    initDragStart();
                    m_CurrentRotateDeltaDeg = 0.0f;

                    const int idx = static_cast<int>(m_ActiveHandle) - static_cast<int>(HandleID::AxisX);
                    m_DragAxis0 = axes[idx];
                }
            }

            // 2. Aktif Sürükleme Hesaplaması
            if (m_ActiveHandle != HandleID::None && mouseDown && !m_DragEntities.empty())
            {
                // Ekran uzayında merkez etrafındaki teğetsel fare hareketinden pürüzsüz açısal delta hesapla
                const glm::vec2 toStart(m_DragStartMousePos.x - originScreen.x, m_DragStartMousePos.y - originScreen.y);
                glm::vec2 tangent(-toStart.y, toStart.x);
                if (glm::length(tangent) < 1.0f) {
                    tangent = glm::vec2(1.0f, 0.0f);
                }
                else {
                    tangent = glm::normalize(tangent);
                }

                // Kamera bakış yönüne göre dönüş işaretini düzelt
                const float viewSign = (glm::dot(m_DragAxis0, viewDirToCenter) < 0.0f) ? 1.0f : -1.0f;
                const glm::vec2 mouseDelta(mousePos.x - m_DragStartMousePos.x, mousePos.y - m_DragStartMousePos.y);
                float angleDeg = glm::dot(mouseDelta, tangent) * 0.65f * viewSign;

                if (snapActive)
                {
                    angleDeg = snap_value(angleDeg, settings.rotate_snap_deg);
                }

                m_CurrentRotateDeltaDeg = angleDeg;
                const float angleRad = glm::radians(angleDeg);

                const glm::mat4 deltaRotMat4 = glm::rotate(glm::mat4(1.0f), angleRad, m_DragAxis0);
                const glm::mat3 deltaRotMat3 = glm::mat3(deltaRotMat4);

                for (size_t i = 0; i < m_DragEntities.size(); ++i)
                {
                    entity ent = m_DragEntities[i];
                    auto* t = reg.get_component<transform_component>(ent);
                    if (!t) continue;

                    // Rotasyon etrafında dönme (Orbiting world position around selection center)
                    const glm::vec3 offset = m_DragStartWorldPositions[i] - m_DragStartWorldPos;
                    const glm::vec3 rotatedOffset = deltaRotMat3 * offset;
                    const glm::vec3 targetWorld = m_DragStartWorldPos + rotatedOffset;

                    glm::mat4 invParent(1.0f);
                    if (const auto* hc = reg.get_component<hierarchy_component>(ent))
                    {
                        if (hc->parent != 0)
                        {
                            invParent = glm::inverse(ecs_transform::get_world_matrix(hc->parent, reg));
                        }
                    }

                    t->position = glm::vec3(invParent * glm::vec4(targetWorld, 1.0f));

                    // Yerel yönelim rotasyonu
                    const glm::mat3 startRotMat = euler_to_mat3(m_DragStartRotations[i]);
                    const glm::mat3 finalRotMat = deltaRotMat3 * startRotMat;
                    t->rotation = mat3_to_euler(finalRotMat);
                }

                if (m_DragEntities.size() > 1)
                {
                    ImGui::SetTooltip("Rotating %zu Entities\nAngle: %+.1f deg", m_DragEntities.size(), m_CurrentRotateDeltaDeg);
                }
                else if (primaryTransform)
                {
                    ImGui::SetTooltip("Rotate Delta: %+.1f deg\nEuler (rad): (%.2f, %.2f, %.2f)",
                        m_CurrentRotateDeltaDeg, primaryTransform->rotation.x, primaryTransform->rotation.y, primaryTransform->rotation.z);
                }
            }

            // 3. Çizim: Dış referans çemberi ve X, Y, Z dönüş halkaları
            dl->AddCircle(originScreen, settings.handle_size * 0.98f, IM_COL32(220, 225, 235, 70), 48, 1.4f);

            for (int i = 0; i < 3; ++i)
            {
                const HandleID id = static_cast<HandleID>(static_cast<int>(HandleID::AxisX) + i);
                const bool hot = (m_HoveredHandle == id) || (m_ActiveHandle == id);

                glm::vec3 u, v;
                build_basis(axes[i], u, v);
                const float step = glm::two_pi<float>() / static_cast<float>(ringSegments);

                for (int s = 0; s < ringSegments; ++s)
                {
                    const float a0 = static_cast<float>(s) * step;
                    const float a1 = static_cast<float>(s + 1) * step;

                    const glm::vec3 dir0 = u * std::cos(a0) + v * std::sin(a0);
                    const glm::vec3 dir1 = u * std::cos(a1) + v * std::sin(a1);

                    const glm::vec3 p0 = originWorld + dir0 * ringRadius;
                    const glm::vec3 p1 = originWorld + dir1 * ringRadius;

                    ImVec2 sp0, sp1;
                    if (project_world_to_screen(p0, viewProj, viewportPos, viewportSize, sp0) &&
                        project_world_to_screen(p1, viewProj, viewportPos, viewportSize, sp1))
                    {
                        // Kameraya bakan ön yarı-küreyi parlak, arka yarı-küreyi yarı saydam çiz
                        const bool frontFacing = (glm::dot(dir0, -viewDirToCenter) >= -0.15f);
                        ImU32 segCol = hot ? hoverColor : axisColors[i];
                        if (!hot && !frontFacing)
                        {
                            segCol = (segCol & 0x00FFFFFF) | (90u << 24);
                        }
                        dl->AddLine(sp0, sp1, segCol, hot ? 3.6f : (frontFacing ? 2.5f : 1.4f));
                    }
                }
            }

            dl->AddCircleFilled(originScreen, 4.0f, IM_COL32(240, 240, 245, 220), 12);
        }

        // ====================================================================
        // MOD C: SCALE (ÖLÇEKLENDİRME - R)
        // ====================================================================
        else if (settings.active_tool == gizmo_tool_mode::scale)
        {
            const glm::vec3 tipWorld[3] = {
                originWorld + axes[0] * gizmoWorldScale,
                originWorld + axes[1] * gizmoWorldScale,
                originWorld + axes[2] * gizmoWorldScale
            };

            ImVec2 tipScreen[3];
            bool tipValid[3];
            for (int i = 0; i < 3; ++i)
            {
                tipValid[i] = project_world_to_screen(tipWorld[i], viewProj, viewportPos, viewportSize, tipScreen[i]);
            }

            // 1. Hover Tespiti
            if (m_ActiveHandle == HandleID::None && m_ActiveLightHandle == -1)
            {
                m_HoveredHandle = HandleID::None;
                float bestDist = 10.0f;

                // Merkez Uniform Scale küpü
                if (std::hypot(mousePos.x - originScreen.x, mousePos.y - originScreen.y) <= 10.0f)
                {
                    m_HoveredHandle = HandleID::Center;
                }
                else
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        if (!tipValid[i]) continue;
                        const float d = dist_point_to_segment_2d(mousePos, originScreen, tipScreen[i]);
                        if (d < bestDist)
                        {
                            bestDist = d;
                            m_HoveredHandle = static_cast<HandleID>(static_cast<int>(HandleID::AxisX) + i);
                        }
                    }
                }

                if (m_HoveredHandle != HandleID::None && mouseClicked)
                {
                    m_ActiveHandle = m_HoveredHandle;
                    initDragStart();

                    if (m_ActiveHandle != HandleID::Center)
                    {
                        const int idx = static_cast<int>(m_ActiveHandle) - static_cast<int>(HandleID::AxisX);
                        m_DragAxis0 = axes[idx];
                        m_DragStartAxisParam = ray_axis_closest_param(mouseRay, originWorld, m_DragAxis0);
                    }
                }
            }

            // 2. Aktif Sürükleme Hesaplaması
            if (m_ActiveHandle != HandleID::None && mouseDown && !m_DragEntities.empty())
            {
                if (m_ActiveHandle == HandleID::Center)
                {
                    const float mouseDelta = (mousePos.x - m_DragStartMousePos.x) - (mousePos.y - m_DragStartMousePos.y);
                    float factor = 1.0f + mouseDelta * 0.0075f;
                    if (snapActive) {
                        factor = snap_value(factor, settings.scale_snap);
                    }
                    factor = std::max(0.01f, factor);

                    for (size_t i = 0; i < m_DragEntities.size(); ++i)
                    {
                        entity ent = m_DragEntities[i];
                        auto* t = reg.get_component<transform_component>(ent);
                        if (!t) continue;

                        const glm::vec3 offset = m_DragStartWorldPositions[i] - m_DragStartWorldPos;
                        const glm::vec3 scaledOffset = offset * factor;
                        const glm::vec3 targetWorld = m_DragStartWorldPos + scaledOffset;

                        glm::mat4 invParent(1.0f);
                        if (const auto* hc = reg.get_component<hierarchy_component>(ent))
                        {
                            if (hc->parent != 0)
                            {
                                invParent = glm::inverse(ecs_transform::get_world_matrix(hc->parent, reg));
                            }
                        }

                        t->position = glm::vec3(invParent * glm::vec4(targetWorld, 1.0f));
                        t->scale = glm::max(glm::vec3(0.001f), m_DragStartScales[i] * factor);
                    }

                    if (m_DragEntities.size() > 1)
                    {
                        ImGui::SetTooltip("Scaling %zu Entities (Uniform)\nFactor: %.2fx", m_DragEntities.size(), factor);
                    }
                    else if (primaryTransform)
                    {
                        ImGui::SetTooltip("Scale: (%.2f, %.2f, %.2f)", primaryTransform->scale.x, primaryTransform->scale.y, primaryTransform->scale.z);
                    }
                }
                else
                {
                    const int idx = static_cast<int>(m_ActiveHandle) - static_cast<int>(HandleID::AxisX);
                    const float curParam = ray_axis_closest_param(mouseRay, m_DragStartWorldPos, m_DragAxis0);
                    float delta = (curParam - m_DragStartAxisParam) / std::max(0.1f, gizmoWorldScale);
                    if (snapActive) {
                        delta = snap_value(delta, settings.scale_snap);
                    }

                    for (size_t i = 0; i < m_DragEntities.size(); ++i)
                    {
                        entity ent = m_DragEntities[i];
                        auto* t = reg.get_component<transform_component>(ent);
                        if (!t) continue;

                        t->scale[idx] = std::max(0.01f, m_DragStartScales[i][idx] + delta);
                    }

                    if (m_DragEntities.size() > 1)
                    {
                        ImGui::SetTooltip("Scaling %zu Entities (Axis %s)\nDelta: %+.2f",
                            m_DragEntities.size(), idx == 0 ? "X" : (idx == 1 ? "Y" : "Z"), delta);
                    }
                    else if (primaryTransform)
                    {
                        ImGui::SetTooltip("Scale: (%.2f, %.2f, %.2f)", primaryTransform->scale.x, primaryTransform->scale.y, primaryTransform->scale.z);
                    }
                }
            }

            // 3. Çizim: X, Y, Z Ölçek Kolları ve Uç Küpleri
            for (int i = 0; i < 3; ++i)
            {
                if (!tipValid[i]) continue;
                const HandleID id = static_cast<HandleID>(static_cast<int>(HandleID::AxisX) + i);
                const bool hot = (m_HoveredHandle == id) || (m_ActiveHandle == id);
                const ImU32 col = hot ? hoverColor : axisColors[i];

                dl->AddLine(originScreen, tipScreen[i], col, hot ? 3.8f : 2.8f);

                const float boxSz = hot ? 6.5f : 5.0f;
                dl->AddRectFilled(
                    ImVec2(tipScreen[i].x - boxSz, tipScreen[i].y - boxSz),
                    ImVec2(tipScreen[i].x + boxSz, tipScreen[i].y + boxSz),
                    col, 1.5f);
                dl->AddRect(
                    ImVec2(tipScreen[i].x - boxSz, tipScreen[i].y - boxSz),
                    ImVec2(tipScreen[i].x + boxSz, tipScreen[i].y + boxSz),
                    IM_COL32(20, 22, 28, 220), 1.5f, 0, 1.2f);
            }

            // Merkez Uniform Scale Küpü
            const bool centerHot = (m_HoveredHandle == HandleID::Center) || (m_ActiveHandle == HandleID::Center);
            const float cBox = centerHot ? 7.5f : 6.0f;
            dl->AddRectFilled(
                ImVec2(originScreen.x - cBox, originScreen.y - cBox),
                ImVec2(originScreen.x + cBox, originScreen.y + cBox),
                centerHot ? hoverColor : IM_COL32(245, 245, 250, 245), 1.5f);
            dl->AddRect(
                ImVec2(originScreen.x - cBox, originScreen.y - cBox),
                ImVec2(originScreen.x + cBox, originScreen.y + cBox),
                IM_COL32(20, 22, 28, 235), 1.5f, 0, 1.5f);
        }
    }

    // ========================================================================
    // 2B OYUN İÇİ UI KATMANI VE CANVAS GİZMO ÇİZİMİ
    // ========================================================================
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

            // 1. Ebeveyn (Parent) Ekran Sınırlarını Belirle (Canvas veya Ebeveyn RectTransform)
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

            // 2. Unity Modeli Anchor ve Pivot Ekran Koordinatı Hesabı
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

            // C. UI Elemanının Sınır / Çerçeve Çizgisi ve Görsel Anchor Gizmoları
            if (!isSelected)
            {
                // Seçili olmayan tüm UI elemanlarının etrafına ince sınır çizgisi (Gizmo outline)
                dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(110, 165, 235, 90), 0.0f, 0, 1.0f);
            }
            else
            {
                // Seçili eleman için belirgin mavi çerçeve
                dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(65, 175, 255, 240), 0.0f, 0, 1.8f);

                // --- GÖRSEL ANCHOR VE PIVOT GIZMOLARI (UNITY TARZI SARI TUTAMAÇLAR) ---
                const glm::vec2 anchorPoint(anchorMinX + anchorW * 0.5f, anchorTopY + anchorH * 0.5f);

                // Kesikli kılavuz çizgi: Anchor noktasından Pivot noktasına
                dl->AddLine(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(pivotScreen.x, pivotScreen.y), IM_COL32(255, 230, 70, 190), 1.2f);

                // 4 Sarı Üçgen (Anchor Tutamaç Göstergeleri)
                const float tS = 7.0f;
                const ImU32 triFill = IM_COL32(255, 230, 70, 245);
                const ImU32 triBorder = IM_COL32(20, 22, 28, 220);

                // Üst üçgen
                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), triBorder, 1.0f);

                // Alt üçgen
                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triBorder, 1.0f);

                // Sol üçgen
                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x - tS, anchorPoint.y - tS), ImVec2(anchorPoint.x - tS, anchorPoint.y + tS), triBorder, 1.0f);

                // Sağ üçgen
                dl->AddTriangleFilled(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triFill);
                dl->AddTriangle(ImVec2(anchorPoint.x, anchorPoint.y), ImVec2(anchorPoint.x + tS, anchorPoint.y - tS), ImVec2(anchorPoint.x + tS, anchorPoint.y + tS), triBorder, 1.0f);

                // Mavi Pivot Merkezi Noktası
                dl->AddCircleFilled(ImVec2(pivotScreen.x, pivotScreen.y), 4.0f, IM_COL32(65, 185, 255, 255));
                dl->AddCircle(ImVec2(pivotScreen.x, pivotScreen.y), 4.0f, IM_COL32(20, 22, 28, 220), 0, 1.2f);
            }

            // D. Scene Viewport üzerinden doğrudan UI elemanına tıklayarak seçme
            const ImVec2 mousePos = ImGui::GetIO().MousePos;
            const bool isLeftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            if (isLeftClicked && !is_interacting() && m_HoveredHandle == HandleID::None)
            {
                if (mousePos.x >= x0 && mousePos.x <= x1 && mousePos.y >= y0 && mousePos.y <= y1)
                {
                    engineCtx.vestaSys.selection().select_entity(e);
                }
            }
        }
    }

    // ========================================================================
    // 2B UI DİKDÖRTGEN BOYUTLANDIRMA VE TAŞIMA ARACI (RECT TOOL)
    // ========================================================================
    void scene_gizmo_overlay::render_rect_tool(
        EngineContext& engineCtx,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize)
    {
        scene& activeScene = engineCtx.session.editor_scene();
        registry& reg = activeScene.get_registry();
        const auto& selectedEntities = engineCtx.vestaSys.selection().get_selected_entities();
        if (selectedEntities.empty())
        {
            m_ActiveHandle = HandleID::None;
            m_HoveredHandle = HandleID::None;
            return;
        }

        entity selectedEnt = *selectedEntities.begin();

        if (selectedEnt == 0 || !reg.has_component<rect_transform_component>(selectedEnt))
        {
            m_ActiveHandle = HandleID::None;
            m_HoveredHandle = HandleID::None;
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
        auto drawHandle = [&](HandleID id, const ImVec2& pos, ImGuiMouseCursor cursor) -> bool
        {
            const bool hovered = (std::abs(mousePos.x - pos.x) <= hRadius + 2.0f && std::abs(mousePos.y - pos.y) <= hRadius + 2.0f);
            if (hovered && m_ActiveHandle == HandleID::None)
            {
                m_HoveredHandle = id;
                ImGui::SetMouseCursor(cursor);
            }

            const bool hot = (m_HoveredHandle == id || m_ActiveHandle == id);
            dl->AddRectFilled(ImVec2(pos.x - hRadius, pos.y - hRadius), ImVec2(pos.x + hRadius, pos.y + hRadius), hot ? handleHoverCol : handleFillCol, 1.0f);
            dl->AddRect(ImVec2(pos.x - hRadius, pos.y - hRadius), ImVec2(pos.x + hRadius, pos.y + hRadius), handleOutlineCol, 1.0f, 0, 1.2f);
            return hot;
        };

        if (m_ActiveHandle == HandleID::None)
        {
            m_HoveredHandle = HandleID::None;
        }

        // 8 Tutamaç: 4 Köşe + 4 Kenar
        drawHandle(HandleID::RectTopLeft, ImVec2(minX, minY), ImGuiMouseCursor_ResizeNWSE);
        drawHandle(HandleID::RectTopRight, ImVec2(maxX, minY), ImGuiMouseCursor_ResizeNESW);
        drawHandle(HandleID::RectBottomLeft, ImVec2(minX, maxY), ImGuiMouseCursor_ResizeNESW);
        drawHandle(HandleID::RectBottomRight, ImVec2(maxX, maxY), ImGuiMouseCursor_ResizeNWSE);

        drawHandle(HandleID::RectLeft, ImVec2(minX, (minY + maxY) * 0.5f), ImGuiMouseCursor_ResizeEW);
        drawHandle(HandleID::RectRight, ImVec2(maxX, (minY + maxY) * 0.5f), ImGuiMouseCursor_ResizeEW);
        drawHandle(HandleID::RectTop, ImVec2((minX + maxX) * 0.5f, minY), ImGuiMouseCursor_ResizeNS);
        drawHandle(HandleID::RectBottom, ImVec2((minX + maxX) * 0.5f, maxY), ImGuiMouseCursor_ResizeNS);

        // Kutu içi gövde hover (Taşıma)
        if (m_HoveredHandle == HandleID::None)
        {
            if (mousePos.x >= minX && mousePos.x <= maxX && mousePos.y >= minY && mousePos.y <= maxY)
            {
                m_HoveredHandle = HandleID::RectBody;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            }
        }

        // 2. Etkileşim ve Sürükleme Mantığı
        if (m_ActiveHandle == HandleID::None)
        {
            if (isMouseDown && m_HoveredHandle != HandleID::None)
            {
                m_ActiveHandle = m_HoveredHandle;
                m_DragEntity = selectedEnt;
                m_DragStartPosition = glm::vec3(rt->anchored_position.x, rt->anchored_position.y, 0.0f);
                m_DragStartScale = glm::vec3(rt->size.x, rt->size.y, 1.0f);
                m_DragStartMousePos = glm::vec2(mousePos.x, mousePos.y);
            }
        }
        else if (m_ActiveHandle >= HandleID::RectBody && m_ActiveHandle <= HandleID::RectBottom)
        {
            if (!isMouseDown)
            {
                m_ActiveHandle = HandleID::None;
                m_HoveredHandle = HandleID::None;
            }
            else
            {
                const glm::vec2 mouseDelta(mousePos.x - m_DragStartMousePos.x, mousePos.y - m_DragStartMousePos.y);

                if (m_ActiveHandle == HandleID::RectBody)
                {
                    rt->anchored_position.x = m_DragStartPosition.x + mouseDelta.x;
                    rt->anchored_position.y = m_DragStartPosition.y - mouseDelta.y;
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                }
                else if (m_ActiveHandle == HandleID::RectRight)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x + mouseDelta.x * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                }
                else if (m_ActiveHandle == HandleID::RectLeft)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x - mouseDelta.x * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                }
                else if (m_ActiveHandle == HandleID::RectBottom)
                {
                    rt->size.y = std::max(10.0f, m_DragStartScale.y + mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                else if (m_ActiveHandle == HandleID::RectTop)
                {
                    rt->size.y = std::max(10.0f, m_DragStartScale.y - mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                }
                else if (m_ActiveHandle == HandleID::RectBottomRight)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x + mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y + mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                }
                else if (m_ActiveHandle == HandleID::RectTopLeft)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x - mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y - mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                }
                else if (m_ActiveHandle == HandleID::RectTopRight)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x + mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y - mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
                }
                else if (m_ActiveHandle == HandleID::RectBottomLeft)
                {
                    rt->size.x = std::max(10.0f, m_DragStartScale.x - mouseDelta.x * 2.0f);
                    rt->size.y = std::max(10.0f, m_DragStartScale.y + mouseDelta.y * 2.0f);
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
                }
            }
        }
    }

} // namespace vesta

