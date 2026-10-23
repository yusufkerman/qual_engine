#pragma once

/**
 * @file gizmo_math_utils.h
 * @brief Sahne içi gizmo manipülatörleri için 3B izdüşüm, ışın-kesişim ve dönüşüm matematiği yardımcıları.
 */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include "../../../ECS/entity.h"
#include "../../../ECS/registry.h"
#include "../../../ECS/components.h"
#include "../../../ECS/transform_utils.h"

namespace vesta::gizmo_math
{
    // ========================================================================
    // RENK SABİTLERİ (AXIS & HOVER COLORS)
    // ========================================================================
    inline constexpr ImU32 kColorAxisX = IM_COL32(238, 62, 66, 255);   // X - Kırmızı
    inline constexpr ImU32 kColorAxisY = IM_COL32(72, 226, 88, 255);   // Y - Yeşil
    inline constexpr ImU32 kColorAxisZ = IM_COL32(64, 138, 250, 255);  // Z - Mavi
    inline constexpr ImU32 kColorHover = IM_COL32(255, 232, 68, 255);  // Vurgu Sarısı

    inline const ImU32 kAxisColors[3] = { kColorAxisX, kColorAxisY, kColorAxisZ };

    inline const ImU32 kPlaneFillColors[3] = {
        IM_COL32(64, 138, 250, 75),  // XY - Mavi tonu
        IM_COL32(72, 226, 88, 75),   // XZ - Yeşil tonu
        IM_COL32(238, 62, 66, 75)    // YZ - Kırmızı tonu
    };

    inline const ImU32 kPlaneBorderColors[3] = {
        IM_COL32(64, 138, 250, 210),
        IM_COL32(72, 226, 88, 210),
        IM_COL32(238, 62, 66, 210)
    };

    // ========================================================================
    // 3B IŞIN (RAY) YAPISI
    // ========================================================================
    struct Ray3D
    {
        glm::vec3 origin{ 0.0f };
        glm::vec3 dir{ 0.0f, 0.0f, -1.0f };
    };

    /**
     * @brief Dünya uzayındaki bir 3B noktayı viewport ekran koordinatına izdüşürür.
     */
    inline bool project_world_to_screen(
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
    inline Ray3D screen_to_world_ray(
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
    inline float dist_point_to_segment_2d(const ImVec2& p, const ImVec2& a, const ImVec2& b)
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
    inline float ray_axis_closest_param(
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
    inline bool ray_plane_intersect(
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
    inline float snap_value(float val, float step)
    {
        if (step <= 1e-4f) {
            return val;
        }
        return std::round(val / step) * step;
    }

    /**
     * @brief Euler açılarından (Rx * Ry * Rz) 3x3 rotasyon matrisini üretir.
     */
    inline glm::mat3 euler_to_mat3(const glm::vec3& rot)
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
    inline glm::vec3 mat3_to_euler(const glm::mat3& R)
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
    inline void build_basis(const glm::vec3& normal, glm::vec3& outRight, glm::vec3& outUp)
    {
        const glm::vec3 n = glm::length(normal) > 1e-5f ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 helper = (std::abs(n.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        outRight = glm::normalize(glm::cross(helper, n));
        outUp = glm::normalize(glm::cross(n, outRight));
    }

    /**
     * @brief Bir varlığın dünya pozisyonunu ebeveyn hiyerarşisini hesaba katarak yerel uzayda günceller (DRY yardımcısı).
     */
    inline void apply_entity_world_position(entity ent, const glm::vec3& targetWorld, registry& reg)
    {
        auto* t = reg.get_component<transform_component>(ent);
        if (!t) return;

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
} // namespace vesta::gizmo_math

