/**
 * @file gizmo_system.cpp
 * @brief Gizmo geometrilerinin üretimi ve OpenGL toplu çizim uygulaması.
 */

#include "gizmo_system.h"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <cmath>

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

    m_AllocatedCapacity = 10000;
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
    m_LineVertices.push_back({ start, color });
    m_LineVertices.push_back({ end, color });
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
    constexpr int segments = 32;
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

void gizmo_system::Flush(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID)
{
    if (shaderID == 0 || m_LineVertices.empty())
    {
        m_LineVertices.clear();
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

    // 3. Render durumlarını yapılandır
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(2.0f);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);

    // 4. Veri yükleme
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    const size_t vertexCount = m_LineVertices.size();

    if (vertexCount > m_AllocatedCapacity)
    {
        m_AllocatedCapacity = vertexCount * 2;
        glBufferData(GL_ARRAY_BUFFER, m_AllocatedCapacity * sizeof(GizmoVertex), nullptr, GL_DYNAMIC_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * sizeof(GizmoVertex), m_LineVertices.data());

    // 5. Çizim çağrısı
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));

    // 6. Durumları varsayılana geri yükle
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glLineWidth(1.0f);

    m_LineVertices.clear();
}

void gizmo_system::Clear()
{
    m_LineVertices.clear();
}