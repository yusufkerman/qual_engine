#pragma once
#include <glm/glm.hpp>
#include <array>

struct plane
{
    glm::vec3 normal;
    float d;

    float distance(const glm::vec3& p) const
    {
        return glm::dot(normal, p) + d;
    }
};

struct frustum
{
    std::array<plane, 6> planes;

    // Sorumlulukların ayrılması için culling kontrolü buraya taşındı
    bool is_inside(const glm::vec3& worldCenter, float radius) const
    {
        for (const auto& plane : planes)
        {
            if (plane.distance(worldCenter) < -radius)
            {
                return false; // Eğer bir düzlemin bile arkasındaysa dışarıdadır
            }
        }
        return true; // Tüm düzlemlerin içindeyse görünürdür
    }
};

inline frustum buildFrustum(const glm::mat4& m)
{
    frustum f;

    // LEFT
    f.planes[0].normal = glm::vec3(
        m[0][3] + m[0][0],
        m[1][3] + m[1][0],
        m[2][3] + m[2][0]
    );
    f.planes[0].d = m[3][3] + m[3][0];

    // RIGHT
    f.planes[1].normal = glm::vec3(
        m[0][3] - m[0][0],
        m[1][3] - m[1][0],
        m[2][3] - m[2][0]
    );
    f.planes[1].d = m[3][3] - m[3][0];

    // BOTTOM
    f.planes[2].normal = glm::vec3(
        m[0][3] + m[0][1],
        m[1][3] + m[1][1],
        m[2][3] + m[2][1]
    );
    f.planes[2].d = m[3][3] + m[3][1];

    // TOP  🔥 KRİTİK NOKTA
    f.planes[3].normal = glm::vec3(
        m[0][3] - m[0][1],
        m[1][3] - m[1][1],
        m[2][3] - m[2][1]
    );
    f.planes[3].d = m[3][3] - m[3][1];

    // NEAR
    f.planes[4].normal = glm::vec3(
        m[0][3] + m[0][2],
        m[1][3] + m[1][2],
        m[2][3] + m[2][2]
    );
    f.planes[4].d = m[3][3] + m[3][2];

    // FAR
    f.planes[5].normal = glm::vec3(
        m[0][3] - m[0][2],
        m[1][3] - m[1][2],
        m[2][3] - m[2][2]
    );
    f.planes[5].d = m[3][3] - m[3][2];

    // normalize
    for (auto& p : f.planes)
    {
        float len = glm::length(p.normal);
        if (len > 0.0001f)
        {
            p.normal /= len;
            p.d /= len;
        }
    }

    return f;
}