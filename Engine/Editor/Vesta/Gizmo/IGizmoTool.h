#pragma once

/**
 * @file IGizmoTool.h
 * @brief Sahne içi manipülasyon araçları için ortak arayüz ve bağlam veri yapıları.
 */

#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>
#include "../../../ECS/entity.h"
#include "../../../ECS/registry.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"

struct EngineContext;

namespace vesta
{
    /**
     * @struct gizmo_tool_context
     * @brief Bir gizmo aracının fare etkileşimini ve çizimini icra edebilmesi için gerekli parametre paketi.
     */
    struct gizmo_tool_context
    {
        EngineContext& engineCtx;
        const glm::mat4& view;
        const glm::mat4& proj;
        const glm::mat4& viewProj;
        const glm::mat4& invViewProj;
        const glm::vec3& camPos;
        float camFovDeg;
        const glm::vec2& viewportPos;
        const glm::vec2& viewportSize;
        const ImVec2& mousePos;
        bool isMouseDown;
        bool isMouseClicked;
        bool snapActive;
        float gizmoWorldScale;
        glm::vec3 originWorld;
        ImVec2 originScreen;
        glm::mat3 basis; // axes[0] = X, axes[1] = Y, axes[2] = Z
        const std::vector<entity>& activeEntities;
        entity primaryEntity;
        ImDrawList* drawList;
    };

    /**
     * @struct gizmo_drag_state
     * @brief Çoklu varlık seçimi ve dönüşüm işlemlerinde sürükleme başlangıç durumunu saklayan yardımcı yapı (DRY).
     */
    struct gizmo_drag_state
    {
        std::vector<entity> dragEntities;
        std::vector<glm::vec3> startLocalPositions;
        std::vector<glm::vec3> startWorldPositions;
        std::vector<glm::vec3> startRotations;
        std::vector<glm::vec3> startScales;

        entity primaryEntity = 0;
        glm::vec3 startWorldPos{ 0.0f };
        glm::vec3 startPosition{ 0.0f };
        glm::vec3 startRotation{ 0.0f };
        glm::vec3 startScale{ 1.0f };
        glm::vec2 startMousePos{ 0.0f };

        void capture(const std::vector<entity>& activeEntities, entity primary, const glm::vec3& originWorld, const ImVec2& mousePos, registry& reg)
        {
            dragEntities.clear();
            startLocalPositions.clear();
            startWorldPositions.clear();
            startRotations.clear();
            startScales.clear();

            for (entity ent : activeEntities)
            {
                auto* t = reg.get_component<transform_component>(ent);
                if (t)
                {
                    dragEntities.push_back(ent);
                    startLocalPositions.push_back(t->position);
                    startRotations.push_back(t->rotation);
                    startScales.push_back(t->scale);
                    const glm::mat4 wMat = ecs_transform::get_world_matrix(ent, reg);
                    startWorldPositions.push_back(glm::vec3(wMat[3]));
                }
            }

            primaryEntity = primary;
            auto* primaryTransform = reg.get_component<transform_component>(primary);
            startPosition = primaryTransform ? primaryTransform->position : glm::vec3(0.0f);
            startScale = primaryTransform ? primaryTransform->scale : glm::vec3(1.0f);
            startRotation = primaryTransform ? primaryTransform->rotation : glm::vec3(0.0f);
            startWorldPos = originWorld;
            startMousePos = glm::vec2(mousePos.x, mousePos.y);
        }

        void clear()
        {
            dragEntities.clear();
            startLocalPositions.clear();
            startWorldPositions.clear();
            startRotations.clear();
            startScales.clear();
            primaryEntity = 0;
        }

        [[nodiscard]] bool empty() const noexcept { return dragEntities.empty(); }
    };

    /**
     * @class IGizmoTool
     * @brief Tüm transform ve manipülasyon araçlarının türediği soyut taban sınıf.
     */
    class IGizmoTool
    {
    public:
        virtual ~IGizmoTool() = default;

        /**
         * @brief Aracın o anda bir eksen veya tutamaç üzerinde aktif sürükleme yapıp yapmadığını döndürür.
         */
        [[nodiscard]] virtual bool is_interacting() const noexcept = 0;

        /**
         * @brief Aktif sürükleme durumunu temizler.
         */
        virtual void reset_interaction() = 0;

        /**
         * @brief Aracın fare kesişim testlerini ve ImGui çizimini icra eder.
         */
        virtual void process(const gizmo_tool_context& ctx) = 0;
    };
} // namespace vesta

