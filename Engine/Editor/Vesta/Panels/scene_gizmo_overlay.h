#pragma once

/**
 * @file scene_gizmo_overlay.h
 * @brief Sahne görünümü (Scene Viewport) üzerindeki etkileşimli Transform (Translate/Rotate/Scale) araçlarını,
 *        Unity tarzı etkileşimli ışık menzil/koni tutamaçlarını, ses alanı tutamaçlarını,
 *        görünmez nesne billboard ikonlarını ve üst Gizmo ayar araç çubuğunu yöneten orkestratör sınıf.
 */

#include <glm/glm.hpp>
#include "../../../ECS/entity.h"
#include "../../../Renderer/Debug/gizmo_settings.h"
#include "../Gizmo/gizmo_translation_tool.h"
#include "../Gizmo/gizmo_rotation_tool.h"
#include "../Gizmo/gizmo_scale_tool.h"
#include "../Gizmo/gizmo_rect_tool.h"
#include "../Gizmo/gizmo_light_tool.h"
#include "../Gizmo/gizmo_audio_tool.h"

struct EngineContext;
struct EditorContext;
class registry;

namespace vesta
{
    /**
     * @class scene_gizmo_overlay
     * @brief İlgili transform, ışık ve ses manipülasyon araçlarını koordine eden,
     *        araç çubuğu ve istatistik panellerini yöneten ana orkestrasyon sınıfı.
     */
    class scene_gizmo_overlay
    {
    public:
        scene_gizmo_overlay() = default;
        ~scene_gizmo_overlay() = default;

        /**
         * @brief Sahne görüntüsü üzerine tüm 2B/3B etkileşimli gizmo katmanlarını ve üst araç çubuğunu çizer.
         *
         * @param engineCtx Motor bağlamı.
         * @param editorCtx Editör bağlamı.
         * @param viewportPos Sahne görüntüsünün sol-üst ekran koordinatı.
         * @param viewportSize Sahne görüntüsünün piksel boyutu.
         * @param isViewportFocused Sahne penceresinin odakta olup olmadığı.
         * @param isViewportHovered Fare imlecinin sahne penceresi üzerinde olup olmadığı.
         */
        void render(
            EngineContext& engineCtx,
            EditorContext& editorCtx,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize,
            bool isViewportFocused,
            bool isViewportHovered);

        /**
         * @brief Kullanıcı o anda bir gizmo eksenini, ışık tutamacını veya ses alanını sürüklüyor mu?
         */
        [[nodiscard]] bool is_interacting() const noexcept
        {
            return m_TranslationTool.is_interacting() ||
                   m_RotationTool.is_interacting() ||
                   m_ScaleTool.is_interacting() ||
                   m_RectTool.is_interacting() ||
                   m_LightTool.is_interacting() ||
                   m_AudioTool.is_interacting();
        }

        /**
         * @brief Bir varlığın sahne/hiyerarşi ikon tipini bileşenlerine veya özel atamasına göre çözümler.
         */
        static entity_icon_type resolve_entity_icon(entity e, const registry& reg, const gizmo_settings& settings);

        /**
         * @brief Hiyerarşi veya Inspector içerisinde tek bir ikon rozeti çizer.
         */
        static void draw_icon_badge_2d(entity_icon_type type, const glm::vec2& center, float radius, const glm::vec3& customTint = glm::vec3(1.0f));

        /**
         * @brief İkon tipinin kullanıcı dostu görünen adını döndürür.
         */
        static const char* get_icon_type_name(entity_icon_type type);

    private:
        void process_shortcuts(
            EngineContext& engineCtx,
            EditorContext& editorCtx,
            bool isViewportFocused,
            bool isViewportHovered);

        void render_ui_overlay(
            EngineContext& engineCtx,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

        void render_toolbar(
            EngineContext& engineCtx,
            EditorContext& editorCtx,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

        void render_stats_overlay(
            EngineContext& engineCtx,
            EditorContext& editorCtx,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

        void render_billboard_icons(
            EngineContext& engineCtx,
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::vec3& camPos,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize,
            bool allowClick);

        void render_transform_gizmo(
            EngineContext& engineCtx,
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::vec3& camPos,
            float camFovDeg,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

    private:
        // Modüler Gizmo Araçları
        gizmo_translation_tool m_TranslationTool;
        gizmo_rotation_tool    m_RotationTool;
        gizmo_scale_tool       m_ScaleTool;
        gizmo_rect_tool        m_RectTool;
        gizmo_light_tool       m_LightTool;
        gizmo_audio_tool       m_AudioTool;

        // Performans / Stats Overlay halka tamponu (Frame Time History)
        static constexpr int kFrameHistoryCount = 90;
        float m_FrameTimeHistoryMs[kFrameHistoryCount]{};
        int m_FrameHistoryOffset = 0;
        float m_SmoothedFps = 60.0f;
        float m_SmoothedFrameMs = 16.67f;
    };
} // namespace vesta
