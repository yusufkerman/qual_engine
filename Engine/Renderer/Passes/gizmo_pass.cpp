/**
 * @file gizmo_pass.cpp
 * @brief Modüler gizmo render geçişi (Collider, Kamera Frustum, Işık Sınırları ve Gameplay Gizmos) uygulaması.
 */

#include "gizmo_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Editor/editor_context.h"
#include "../../Editor/editor_camera.h"
#include "../../Editor/Vesta/vesta.h"
#include "../../ECS/components.h"
#include "../../ECS/transform_utils.h"
#include "../../Systems/physics_api.h"
#include "../../Engine/fly_camera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace
{
    /**
     * @brief Bir varlığın geçerli görünürlük moduna göre çizilip çizilmeyeceğini kontrol eder.
     */
    bool should_draw_for_entity(gizmo_visibility_mode mode, entity e, int selectedID)
    {
        if (mode == gizmo_visibility_mode::off) {
            return false;
        }
        if (mode == gizmo_visibility_mode::always) {
            return true;
        }
        return (selectedID != -1) && (static_cast<int>(e) == selectedID);
    }

    /**
     * @brief Euler dönüş açılarından ışık ileri yön vektörünü hesaplar (light_system ile birebir uyumlu).
     */
    glm::vec3 compute_light_forward(const glm::vec3& rot)
    {
        glm::mat4 m(1.0f);
        m = glm::rotate(m, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
        return glm::normalize(glm::vec3(m * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    }

    /**
     * @brief Çarpıştırıcı (Collider) bileşenlerinin tel kafes sınırlarını çizer.
     */
    void draw_collider_gizmos(EngineContext& ctx, const registry& reg, int selectedID)
    {
        const auto& settings = ctx.gizmoSystem.settings();
        if (settings.collider_mode == gizmo_visibility_mode::off) {
            return;
        }

        for (const auto& [e, collider] : reg.get_all<collider_component>())
        {
            if (!should_draw_for_entity(settings.collider_mode, e, selectedID)) {
                continue;
            }

            const auto* transform = reg.get_component<transform_component>(e);
            if (!transform) {
                continue;
            }

            const auto* rb = reg.get_component<rigidbody_component>(e);
            glm::mat4 modelMatrix(1.0f);
            bool physicsActive = false;

            if (rb && !rb->body_id.IsInvalid())
            {
                modelMatrix = ctx.physicsSystem.api().get_jolt_transform(e);
                physicsActive = true;
            }

            if (!physicsActive)
            {
                const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
                const glm::vec3 worldPos = glm::vec3(worldMat[3]);

                modelMatrix = glm::translate(glm::mat4(1.0f), worldPos);
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
                modelMatrix = glm::rotate(modelMatrix, transform->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            }

            const bool isSelected = (selectedID != -1 && static_cast<int>(e) == selectedID);
            const glm::vec4 gizmoColor = isSelected
                ? glm::vec4(0.18f, 0.95f, 0.32f, 1.0f)
                : glm::vec4(0.18f, 0.80f, 0.30f, 0.48f);

            switch (collider.type)
            {
            case collider_type::box:
                ctx.gizmoSystem.DrawWireCube(modelMatrix, collider.extents, gizmoColor);
                break;

            case collider_type::sphere:
                ctx.gizmoSystem.DrawWireSphere(modelMatrix, collider.extents.x, gizmoColor);
                break;

            default:
                break;
            }
        }
    }

    /**
     * @brief Kamera bileşenlerinin görüş piramidi (Frustum) sınır çizgilerini çizer.
     */
    void draw_camera_gizmos(EngineContext& ctx, const registry& reg, int selectedID)
    {
        const auto& settings = ctx.gizmoSystem.settings();
        if (settings.camera_frustum_mode == gizmo_visibility_mode::off) {
            return;
        }

        float gameAspect = 16.0f / 9.0f;
        if (ctx.gameTarget.height > 0)
        {
            gameAspect = static_cast<float>(ctx.gameTarget.width) /
                static_cast<float>(std::max(1, ctx.gameTarget.height));
        }

        for (const auto& [e, cam] : reg.get_all<camera_component>())
        {
            if (!should_draw_for_entity(settings.camera_frustum_mode, e, selectedID)) {
                continue;
            }

            const auto* transform = reg.get_component<transform_component>(e);
            if (!transform) {
                continue;
            }

            const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
            const glm::vec3 worldPos = glm::vec3(worldMat[3]);

            const glm::vec3 forward = calc_forward(transform->rotation);
            const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::cross(forward, worldUp);
            if (glm::length(right) < 1e-4f) {
                right = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            else {
                right = glm::normalize(right);
            }
            const glm::vec3 up = glm::normalize(glm::cross(right, forward));

            const float visualFar = settings.clamp_camera_frustum_dist
                ? std::min(cam.farPlane, settings.camera_frustum_max_draw_dist)
                : cam.farPlane;

            const bool isSelected = (selectedID != -1 && static_cast<int>(e) == selectedID);
            const glm::vec4 frustumColor = isSelected
                ? glm::vec4(0.92f, 0.94f, 0.98f, 0.92f)
                : glm::vec4(0.75f, 0.78f, 0.85f, 0.45f);

            ctx.gizmoSystem.DrawCameraFrustum(
                worldPos,
                forward,
                right,
                up,
                cam.fov,
                gameAspect,
                cam.nearPlane,
                visualFar,
                frustumColor
            );
        }
    }

    /**
     * @brief Işık bileşenlerinin (Directional, Point, Spot) menzil ve koni sınır çizgilerini çizer.
     */
    void draw_light_gizmos(EngineContext& ctx, const registry& reg, int selectedID)
    {
        const auto& settings = ctx.gizmoSystem.settings();
        if (settings.light_gizmo_mode == gizmo_visibility_mode::off) {
            return;
        }

        for (const auto& [e, light] : reg.get_all<light_component>())
        {
            if (!should_draw_for_entity(settings.light_gizmo_mode, e, selectedID)) {
                continue;
            }

            const auto* transform = reg.get_component<transform_component>(e);
            if (!transform) {
                continue;
            }

            const glm::mat4 worldMat = ecs_transform::get_world_matrix(e, reg);
            const glm::vec3 worldPos = glm::vec3(worldMat[3]);
            const glm::vec3 dir = compute_light_forward(transform->rotation);

            const bool isSelected = (selectedID != -1 && static_cast<int>(e) == selectedID);
            const float baseAlpha = isSelected ? 0.92f : 0.45f;

            // Işık rengini sıcak sarı ile harmanlayarak her arka planda görünür olmasını sağla
            const glm::vec3 BaseTint = glm::mix(glm::clamp(light.color, glm::vec3(0.25f), glm::vec3(1.0f)), glm::vec3(1.0f, 0.88f, 0.32f), 0.35f);
            const glm::vec4 mainColor(BaseTint, baseAlpha);
            const glm::vec4 innerColor(BaseTint * 0.9f + glm::vec3(0.1f), baseAlpha * 0.55f);

            switch (light.type)
            {
            case light_type::point:
            {
                const float range = std::max(0.1f, light.range);
                ctx.gizmoSystem.DrawWireSphere(worldPos, range, mainColor);
                break;
            }
            case light_type::spot:
            {
                const float range = std::max(0.1f, light.range);
                const float outerAngle = std::clamp(light.outer_cone, 1.0f, 85.0f);
                const float innerAngle = std::clamp(light.inner_cone, 0.5f, outerAngle);

                // Dış koni (Outer Cone)
                ctx.gizmoSystem.DrawWireCone(worldPos, dir, range, outerAngle, mainColor, 40);

                // İç koni (Inner Cone) - Daha hafif alfa ile çizilir
                if (std::abs(outerAngle - innerAngle) > 0.5f)
                {
                    ctx.gizmoSystem.DrawWireCone(worldPos, dir, range, innerAngle, innerColor, 32);
                }

                // Merkez menzil eksen çizgisi
                ctx.gizmoSystem.DrawLine(worldPos, worldPos + dir * range, innerColor);
                break;
            }
            case light_type::directional:
            {
                ctx.gizmoSystem.DrawDirectionalLightGizmo(worldPos, dir, 0.65f, 2.2f, mainColor);
                break;
            }
            default:
                break;
            }
        }
    }
}

void gizmo_pass::render(EngineContext& ctx, EditorContext& editorCtx)
{
    if (!editorCtx.camera || !ctx.aria_system) {
        return;
    }

    const auto& settings = ctx.gizmoSystem.settings();
    if (!settings.enabled)
    {
        ctx.gizmoSystem.Clear();
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ctx.editorTarget.fbo);
    glViewport(0, 0, ctx.editorTarget.width, ctx.editorTarget.height);

    const float aspect = static_cast<float>(ctx.editorTarget.width) /
        static_cast<float>(std::max(1, ctx.editorTarget.height));

    const glm::mat4 editorView = editorCtx.camera->getViewMatrix();
    const glm::mat4 editorProj = editorCtx.camera->getProjectionMatrix(aspect);

    const auto& reg = ctx.session.editor_scene().get_registry();
    const int selectedID = ctx.vestaSys.selection().get_first_selected_entity();

    // 1. Bileşen gizmolarını üret
    draw_collider_gizmos(ctx, reg, selectedID);
    draw_camera_gizmos(ctx, reg, selectedID);
    draw_light_gizmos(ctx, reg, selectedID);

    // 2. GPU üzerinde çizdir
    const unsigned int gizmoGLShaderID = ctx.aria_system->get_shader_id(ctx.gizmoShaderHandle);
    ctx.gizmoSystem.Flush(editorView, editorProj, gizmoGLShaderID);

    // 3. Oynanış (Gameplay API) hata ayıklama çizgilerini Sahne görünümünde çizdir
    if (settings.show_gameplay_gizmos_in_scene)
    {
        ctx.gizmoSystem.RenderGameplayGizmos(editorView, editorProj, gizmoGLShaderID);
    }
}

