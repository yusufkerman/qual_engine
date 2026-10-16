#pragma once

/**
 * @file scene_gizmo_overlay.h
 * @brief Sahne görünümü (Scene Viewport) üzerindeki etkileşimli Transform (Translate/Rotate/Scale) araçlarını,
 *        Unity tarzı etkileşimli ışık menzil/koni tutamaçlarını, görünmez nesne billboard ikonlarını ve
 *        üst Gizmo ayar araç çubuğunu yöneten modüler sınıf.
 */

#include <glm/glm.hpp>
#include "../../../ECS/entity.h"
#include "../../../Renderer/Debug/gizmo_settings.h"

struct EngineContext;
struct EditorContext;
class registry;

namespace vesta
{
    /**
     * @class scene_gizmo_overlay
     * @brief ImDrawList ve 3B ışın-düzlem/ışın-doğru kesişim matematiği kullanarak sahne içi manipülasyon araçlarını çizer ve işler.
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
         * @brief Kullanıcı o anda bir gizmo eksenini veya ışık tutamacını sürüklüyor mu?
         */
        [[nodiscard]] bool is_interacting() const noexcept { return m_ActiveHandle != HandleID::None || m_ActiveLightHandle >= 0; }

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
        enum class HandleID : int
        {
            None = 0,
            AxisX = 1,
            AxisY = 2,
            AxisZ = 3,
            PlaneXY = 4,
            PlaneXZ = 5,
            PlaneYZ = 6,
            Center = 7
        };

        void process_shortcuts(EngineContext& engineCtx, bool isViewportFocused, bool isViewportHovered);

        void render_toolbar(
            EngineContext& engineCtx,
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

        void render_interactive_light_handles(
            EngineContext& engineCtx,
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::vec3& camPos,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

        void render_transform_gizmo(
            EngineContext& engineCtx,
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::vec3& camPos,
            float camFovDeg,
            const glm::vec2& viewportPos,
            const glm::vec2& viewportSize);

    private:
        // Transform Gizmo etkileşim durumu
        HandleID m_HoveredHandle = HandleID::None;
        HandleID m_ActiveHandle = HandleID::None;
        entity m_DragEntity = 0;

        std::vector<entity> m_DragEntities;
        std::vector<glm::vec3> m_DragStartLocalPositions;
        std::vector<glm::vec3> m_DragStartWorldPositions;
        std::vector<glm::vec3> m_DragStartRotations;
        std::vector<glm::vec3> m_DragStartScales;

        glm::vec3 m_DragStartPosition{ 0.0f };
        glm::vec3 m_DragStartRotation{ 0.0f };
        glm::vec3 m_DragStartScale{ 1.0f };
        glm::vec3 m_DragStartWorldPos{ 0.0f };

        glm::vec3 m_DragAxis0{ 1.0f, 0.0f, 0.0f };
        glm::vec3 m_DragAxis1{ 0.0f, 1.0f, 0.0f };
        glm::vec3 m_DragPlaneNormal{ 0.0f, 0.0f, 1.0f };
        glm::vec3 m_DragStartHitPoint{ 0.0f };
        float m_DragStartAxisParam = 0.0f;
        glm::vec2 m_DragStartMousePos{ 0.0f };
        float m_CurrentRotateDeltaDeg = 0.0f;

        // Interactive Light Handle etkileşim durumu
        int m_HoveredLightHandle = -1;
        int m_ActiveLightHandle = -1;
        float m_LightDragStartRange = 10.0f;
        float m_LightDragStartOuterCone = 25.0f;
        float m_LightDragStartInnerCone = 15.0f;
    };
}

