/**
 * @file gizmo_system.cpp
 * @brief Gizmo geometrilerinin üretimi ve OpenGL toplu çizim uygulaması.
 */

#include "gizmo_system.h"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace
{
    /**
     * @brief Verilen birim normal vektörüne dik iki ortonormal taban vektörü (tangent, bitangent) üretir.
     */
    void build_orthonormal_basis(const glm::vec3& normal, glm::vec3& outRight, glm::vec3& outUp)
    {
        const glm::vec3 n = glm::length(normal) > 1e-5f ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 helper = (std::abs(n.y) < 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        outRight = glm::normalize(glm::cross(helper, n));
        outUp = glm::normalize(glm::cross(n, outRight));
    }
}

gizmo_system::gizmo_system() = default;

gizmo_system::~gizmo_system()
{
    shutdown();
}

void gizmo_system::init()
{
    if (m_VAO != 0) {
        return;
    }

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    m_AllocatedCapacity = 16384;
    glBufferData(GL_ARRAY_BUFFER, m_AllocatedCapacity * sizeof(GizmoVertex), nullptr, GL_DYNAMIC_DRAW);

    // Konum (Position) niteliği
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // Renk (Color) niteliği
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), reinterpret_cast<void*>(offsetof(GizmoVertex, color)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void gizmo_system::shutdown()
{
    if (m_VAO != 0)
    {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        m_VAO = 0;
        m_VBO = 0;
        m_AllocatedCapacity = 0;
    }
}

void gizmo_system::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    auto& targetBuffer = m_WritingGameplay ? m_GameplayLineVertices : m_LineVertices;
    targetBuffer.push_back({ start, color });
    targetBuffer.push_back({ end, color });
}

void gizmo_system::DrawRay(const glm::vec3& origin, const glm::vec3& direction, const glm::vec4& color)
{
    DrawLine(origin, origin + direction, color);
}

void gizmo_system::DrawArrow(const glm::vec3& start, const glm::vec3& end, float headSize, const glm::vec4& color)
{
    DrawLine(start, end, color);

    const glm::vec3 diff = end - start;
    const float len = glm::length(diff);
    if (len < 1e-4f) {
        return;
    }

    const glm::vec3 dir = diff / len;
    glm::vec3 right, up;
    build_orthonormal_basis(dir, right, up);

    const float actualHead = std::min(headSize, len * 0.45f);
    const glm::vec3 base = end - dir * actualHead;
    const float wing = actualHead * 0.45f;

    DrawLine(end, base + right * wing, color);
    DrawLine(end, base - right * wing, color);
    DrawLine(end, base + up * wing, color);
    DrawLine(end, base - up * wing, color);
}

void gizmo_system::DrawWireCube(const glm::vec3& center, const glm::vec3& scale, const glm::vec4& color)
{
    const glm::vec3 extents = scale * 0.5f;

    const glm::vec3 v[8] = {
        center + glm::vec3(-extents.x, -extents.y, -extents.z),
        center + glm::vec3(extents.x, -extents.y, -extents.z),
        center + glm::vec3(extents.x,  extents.y, -extents.z),
        center + glm::vec3(-extents.x,  extents.y, -extents.z),
        center + glm::vec3(-extents.x, -extents.y,  extents.z),
        center + glm::vec3(extents.x, -extents.y,  extents.z),
        center + glm::vec3(extents.x,  extents.y,  extents.z),
        center + glm::vec3(-extents.x,  extents.y,  extents.z)
    };

    // Alt yüzey
    DrawLine(v[0], v[1], color); DrawLine(v[1], v[2], color);
    DrawLine(v[2], v[3], color); DrawLine(v[3], v[0], color);

    // Üst yüzey
    DrawLine(v[4], v[5], color); DrawLine(v[5], v[6], color);
    DrawLine(v[6], v[7], color); DrawLine(v[7], v[4], color);

    // Dikey birleşimler
    DrawLine(v[0], v[4], color); DrawLine(v[1], v[5], color);
    DrawLine(v[2], v[6], color); DrawLine(v[3], v[7], color);
}

void gizmo_system::DrawWireCube(const glm::mat4& transform, const glm::vec3& extents, const glm::vec4& color)
{
    const glm::vec4 localV[8] = {
        glm::vec4(-extents.x, -extents.y, -extents.z, 1.0f),
        glm::vec4(extents.x, -extents.y, -extents.z, 1.0f),
        glm::vec4(extents.x,  extents.y, -extents.z, 1.0f),
        glm::vec4(-extents.x,  extents.y, -extents.z, 1.0f),
        glm::vec4(-extents.x, -extents.y,  extents.z, 1.0f),
        glm::vec4(extents.x, -extents.y,  extents.z, 1.0f),
        glm::vec4(extents.x,  extents.y,  extents.z, 1.0f),
        glm::vec4(-extents.x,  extents.y,  extents.z, 1.0f)
    };

    glm::vec3 v[8];
    for (int i = 0; i < 8; ++i)
    {
        v[i] = glm::vec3(transform * localV[i]);
    }

    // Taban çizgileri
    DrawLine(v[0], v[1], color); DrawLine(v[1], v[2], color);
    DrawLine(v[2], v[3], color); DrawLine(v[3], v[0], color);

    // Tavan çizgileri
    DrawLine(v[4], v[5], color); DrawLine(v[5], v[6], color);
    DrawLine(v[6], v[7], color); DrawLine(v[7], v[4], color);

    // Dikey bağlantılar
    DrawLine(v[0], v[4], color); DrawLine(v[1], v[5], color);
    DrawLine(v[2], v[6], color); DrawLine(v[3], v[7], color);
}

void gizmo_system::DrawWireSphere(const glm::mat4& transform, float radius, const glm::vec4& color)
{
    constexpr int segments = 36;
    const float step = glm::two_pi<float>() / static_cast<float>(segments);

    auto TransformPoint = [&](const glm::vec3& p) {
        return glm::vec3(transform * glm::vec4(p, 1.0f));
        };

    for (int i = 0; i < segments; ++i)
    {
        const float a0 = static_cast<float>(i) * step;
        const float a1 = static_cast<float>(i + 1) * step;

        const float cos0 = std::cos(a0);
        const float sin0 = std::sin(a0);
        const float cos1 = std::cos(a1);
        const float sin1 = std::sin(a1);

        // XY Düzlemi
        DrawLine(
            TransformPoint(glm::vec3(cos0, sin0, 0.0f) * radius),
            TransformPoint(glm::vec3(cos1, sin1, 0.0f) * radius),
            color
        );

        // XZ Düzlemi
        DrawLine(
            TransformPoint(glm::vec3(cos0, 0.0f, sin0) * radius),
            TransformPoint(glm::vec3(cos1, 0.0f, sin1) * radius),
            color
        );

        // YZ Düzlemi
        DrawLine(
            TransformPoint(glm::vec3(0.0f, cos0, sin0) * radius),
            TransformPoint(glm::vec3(0.0f, cos1, sin1) * radius),
            color
        );
    }
}

void gizmo_system::DrawWireSphere(const glm::vec3& center, float radius, const glm::vec4& color)
{
    DrawWireCircle(center, glm::vec3(1.0f, 0.0f, 0.0f), radius, color, 40);
    DrawWireCircle(center, glm::vec3(0.0f, 1.0f, 0.0f), radius, color, 40);
    DrawWireCircle(center, glm::vec3(0.0f, 0.0f, 1.0f), radius, color, 40);
}

void gizmo_system::DrawWireCircle(
    const glm::vec3& center,
    const glm::vec3& normal,
    float radius,
    const glm::vec4& color,
    int segments)
{
    if (radius <= 0.0f || segments < 3) {
        return;
    }

    glm::vec3 right, up;
    build_orthonormal_basis(normal, right, up);

    const float step = glm::two_pi<float>() / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float a0 = static_cast<float>(i) * step;
        const float a1 = static_cast<float>(i + 1) * step;

        const glm::vec3 p0 = center + (right * std::cos(a0) + up * std::sin(a0)) * radius;
        const glm::vec3 p1 = center + (right * std::cos(a1) + up * std::sin(a1)) * radius;
        DrawLine(p0, p1, color);
    }
}

void gizmo_system::DrawWireArc(
    const glm::vec3& center,
    const glm::vec3& normal,
    const glm::vec3& startDir,
    float angleRad,
    float radius,
    const glm::vec4& color,
    int segments)
{
    if (radius <= 0.0f || segments < 2) {
        return;
    }

    const glm::vec3 n = glm::normalize(normal);
    const glm::vec3 u = glm::normalize(startDir);
    const glm::vec3 v = glm::normalize(glm::cross(n, u));

    const float step = angleRad / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float a0 = static_cast<float>(i) * step;
        const float a1 = static_cast<float>(i + 1) * step;

        const glm::vec3 p0 = center + (u * std::cos(a0) + v * std::sin(a0)) * radius;
        const glm::vec3 p1 = center + (u * std::cos(a1) + v * std::sin(a1)) * radius;
        DrawLine(p0, p1, color);
    }
}

void gizmo_system::DrawWireCone(
    const glm::vec3& apex,
    const glm::vec3& direction,
    float range,
    float halfAngleDeg,
    const glm::vec4& color,
    int segments)
{
    if (range <= 0.01f) {
        return;
    }

    const glm::vec3 dir = glm::length(direction) > 1e-5f ? glm::normalize(direction) : glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right, up;
    build_orthonormal_basis(dir, right, up);

    const float clampedAngle = std::clamp(halfAngleDeg, 1.0f, 85.0f);
    const float halfRad = glm::radians(clampedAngle);

    // Küresel menzil şapkası üzerinde koni taban dairesi:
    // Spot ışık menzili küresel olduğundan taban merkezi cos(theta)*range uzaklığında, yarıçapı sin(theta)*range olur.
    const float baseDist = range * std::cos(halfRad);
    const float baseRadius = range * std::sin(halfRad);
    const glm::vec3 baseCenter = apex + dir * baseDist;

    DrawWireCircle(baseCenter, dir, baseRadius, color, segments);

    // Tepe noktasından taban çemberinin 4 ana kadranına yan kenar çizgileri
    DrawLine(apex, baseCenter + right * baseRadius, color);
    DrawLine(apex, baseCenter - right * baseRadius, color);
    DrawLine(apex, baseCenter + up * baseRadius, color);
    DrawLine(apex, baseCenter - up * baseRadius, color);
}

void gizmo_system::DrawCameraFrustum(
    const glm::vec3& position,
    const glm::vec3& forward,
    const glm::vec3& right,
    const glm::vec3& up,
    float fovDeg,
    float aspect,
    float nearPlane,
    float farPlane,
    const glm::vec4& color)
{
    const float safeNear = std::max(0.05f, nearPlane);
    const float safeFar = std::max(safeNear + 0.1f, farPlane);
    const float safeAspect = std::max(0.1f, aspect);
    const float tanHalfFov = std::tan(glm::radians(std::clamp(fovDeg, 5.0f, 170.0f)) * 0.5f);

    const float nearHalfH = safeNear * tanHalfFov;
    const float nearHalfW = nearHalfH * safeAspect;

    const float farHalfH = safeFar * tanHalfFov;
    const float farHalfW = farHalfH * safeAspect;

    const glm::vec3 nearCenter = position + forward * safeNear;
    const glm::vec3 farCenter  = position + forward * safeFar;

    // Yakın düzlem köşeleri (Near Plane)
    const glm::vec3 nTL = nearCenter + up * nearHalfH - right * nearHalfW;
    const glm::vec3 nTR = nearCenter + up * nearHalfH + right * nearHalfW;
    const glm::vec3 nBR = nearCenter - up * nearHalfH + right * nearHalfW;
    const glm::vec3 nBL = nearCenter - up * nearHalfH - right * nearHalfW;

    // Uzak düzlem köşeleri (Far Plane)
    const glm::vec3 fTL = farCenter + up * farHalfH - right * farHalfW;
    const glm::vec3 fTR = farCenter + up * farHalfH + right * farHalfW;
    const glm::vec3 fBR = farCenter - up * farHalfH + right * farHalfW;
    const glm::vec3 fBL = farCenter - up * farHalfH - right * farHalfW;

    // Yakın düzlem çerçevesi
    DrawLine(nTL, nTR, color);
    DrawLine(nTR, nBR, color);
    DrawLine(nBR, nBL, color);
    DrawLine(nBL, nTL, color);

    // Uzak düzlem çerçevesi
    DrawLine(fTL, fTR, color);
    DrawLine(fTR, fBR, color);
    DrawLine(fBR, fBL, color);
    DrawLine(fBL, fTL, color);

    // Piramit yan kenarları (Near -> Far)
    DrawLine(nTL, fTL, color);
    DrawLine(nTR, fTR, color);
    DrawLine(nBR, fBR, color);
    DrawLine(nBL, fBL, color);

    // Kamera merkezinden yakın düzlem köşelerine ince kılavuz çizgileri
    const glm::vec4 subtleColor(color.r, color.g, color.b, color.a * 0.45f);
    DrawLine(position, nTL, subtleColor);
    DrawLine(position, nTR, subtleColor);
    DrawLine(position, nBR, subtleColor);
    DrawLine(position, nBL, subtleColor);

    // Kamera üst yönünü (Up) belirten küçük üçgen şapka (Unity/Unreal kamera gizmosu tarzı)
    const float hatHeight = nearHalfH * 0.45f;
    const glm::vec3 hatTip = nearCenter + up * (nearHalfH + hatHeight);
    DrawLine(nTL + right * (nearHalfW * 0.35f), hatTip, color);
    DrawLine(nTR - right * (nearHalfW * 0.35f), hatTip, color);
}

void gizmo_system::DrawDirectionalLightGizmo(
    const glm::vec3& position,
    const glm::vec3& direction,
    float radius,
    float rayLength,
    const glm::vec4& color)
{
    const glm::vec3 dir = glm::length(direction) > 1e-5f ? glm::normalize(direction) : glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 right, up;
    build_orthonormal_basis(dir, right, up);

    // Güneş diski
    DrawWireCircle(position, dir, radius, color, 24);

    // Merkez ok ve çevreleyen 8 paralel güneş ışını
    DrawArrow(position, position + dir * (rayLength * 1.25f), rayLength * 0.18f, color);

    constexpr int rayCount = 8;
    for (int i = 0; i < rayCount; ++i)
    {
        const float angle = (glm::two_pi<float>() / static_cast<float>(rayCount)) * static_cast<float>(i);
        const glm::vec3 rayOrigin = position + (right * std::cos(angle) + up * std::sin(angle)) * radius;
        DrawArrow(rayOrigin, rayOrigin + dir * rayLength, rayLength * 0.14f, color);
    }
}

void gizmo_system::ClearGameplay()
{
    m_GameplayLineVertices.clear();
}

void gizmo_system::RenderGameplayGizmos(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID)
{
    DrawVertexBatch(m_GameplayLineVertices, view, projection, shaderID, m_Settings.line_width);
}

void gizmo_system::Flush(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID)
{
    DrawVertexBatch(m_LineVertices, view, projection, shaderID, m_Settings.line_width);
    m_LineVertices.clear();
}

void gizmo_system::Clear()
{
    m_LineVertices.clear();
}

void gizmo_system::DrawVertexBatch(
    const std::vector<GizmoVertex>& vertices,
    const glm::mat4& view,
    const glm::mat4& projection,
    unsigned int shaderID,
    float lineWidth)
{
    if (shaderID == 0 || vertices.empty() || m_VAO == 0)
    {
        return;
    }

    // 1. Programı bağla
    glUseProgram(shaderID);

    // 2. Uniform matrisini yükle
    const int loc = glGetUniformLocation(shaderID, "u_ViewProj");
    if (loc != -1) {
        const glm::mat4 viewProj = projection * view;
        glUniformMatrix4fv(loc, 1, GL_FALSE, &viewProj[0][0]);
    }

    // 3. Render durumlarını yapılandır (Alfa karışımı + derinlik testi)
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(std::clamp(lineWidth, 1.0f, 5.0f));
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);

    // 4. Veri yükleme
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    const size_t vertexCount = vertices.size();

    if (vertexCount > m_AllocatedCapacity)
    {
        m_AllocatedCapacity = vertexCount * 2;
        glBufferData(GL_ARRAY_BUFFER, m_AllocatedCapacity * sizeof(GizmoVertex), nullptr, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(GizmoVertex), vertices.data());

    // 5. Çizim çağrısı
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));
    glBindVertexArray(0);

    // 6. Durumları sıfırla
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glUseProgram(0);
}