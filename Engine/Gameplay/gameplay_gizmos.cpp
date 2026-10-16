/**
 * @file gameplay_gizmos.cpp
 * @brief Gameplay API üzerinden motorun gizmo_system alt sistemine güvenli hata ayıklama çizimi yönlendirmesi.
 */

#include "gameplay_gizmos.h"
#include "gameplay_context.h"
#include "../Engine/engine_context.h"

namespace gameplay
{
    void gizmos::draw_line(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
    {
        if (auto* ctx = context::get_engine_context())
        {
            ctx->gizmoSystem.DrawLine(start, end, color);
        }
    }

    void gizmos::draw_ray(const glm::vec3& origin, const glm::vec3& direction, const glm::vec4& color)
    {
        if (auto* ctx = context::get_engine_context())
        {
            ctx->gizmoSystem.DrawRay(origin, direction, color);
        }
    }

    void gizmos::draw_arrow(const glm::vec3& start, const glm::vec3& end, float head_size, const glm::vec4& color)
    {
        if (auto* ctx = context::get_engine_context())
        {
            ctx->gizmoSystem.DrawArrow(start, end, head_size, color);
        }
    }

    void gizmos::draw_wire_cube(const glm::vec3& center, const glm::vec3& size, const glm::vec4& color)
    {
        if (auto* ctx = context::get_engine_context())
        {
            ctx->gizmoSystem.DrawWireCube(center, size, color);
        }
    }

    void gizmos::draw_wire_sphere(const glm::vec3& center, float radius, const glm::vec4& color)
    {
        if (auto* ctx = context::get_engine_context())
        {
            ctx->gizmoSystem.DrawWireSphere(center, radius, color);
        }
    }

    void gizmos::draw_wire_circle(
        const glm::vec3& center,
        const glm::vec3& normal,
        float radius,
        const glm::vec4& color,
        int segments)
    {
        if (auto* ctx = context::get_engine_context())
        {
            ctx->gizmoSystem.DrawWireCircle(center, normal, radius, color, segments);
        }
    }

    void gizmos::draw_wire_cone(
        const glm::vec3& apex,
        const glm::vec3& direction,
        float range,
        float half_angle_deg,
        const glm::vec4& color)
    {
        if (auto* ctx = context::get_engine_context())
        {
            ctx->gizmoSystem.DrawWireCone(apex, direction, range, half_angle_deg, color);
        }
    }
}

