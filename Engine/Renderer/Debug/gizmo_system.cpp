#include "gizmo_system.h"
#include <glad/glad.h>

#include <iostream>

gizmo_system::gizmo_system() {}

gizmo_system::~gizmo_system()
{
    shutdown();
}

void gizmo_system::init()
{
    if (m_VAO != 0) return;

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // Başlangıçta boş bir dinamik buffer ayırıyoruz
    glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(GizmoVertex), nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GizmoVertex), (void*)offsetof(GizmoVertex, color));
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
    }
}

void gizmo_system::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
{
    m_LineVertices.push_back({ start, color });
    m_LineVertices.push_back({ end, color });
}

void gizmo_system::DrawWireCube(const glm::vec3& center, const glm::vec3& scale, const glm::vec4& color)
{
    glm::vec3 extents = scale * 0.5f;

    // 8 Köşe noktası hesaplanır
    glm::vec3 v[8] = {
        center + glm::vec3(-extents.x, -extents.y, -extents.z),
        center + glm::vec3(extents.x, -extents.y, -extents.z),
        center + glm::vec3(extents.x,  extents.y, -extents.z),
        center + glm::vec3(-extents.x,  extents.y, -extents.z),
        center + glm::vec3(-extents.x, -extents.y,  extents.z),
        center + glm::vec3(extents.x, -extents.y,  extents.z),
        center + glm::vec3(extents.x,  extents.y,  extents.z),
        center + glm::vec3(-extents.x,  extents.y,  extents.z)
    };

    // Alt kare çizgileri
    DrawLine(v[0], v[1], color); DrawLine(v[1], v[2], color);
    DrawLine(v[2], v[3], color); DrawLine(v[3], v[0], color);

    // Üst kare çizgileri
    DrawLine(v[4], v[5], color); DrawLine(v[5], v[6], color);
    DrawLine(v[6], v[7], color); DrawLine(v[7], v[4], color);

    // Dikey birleştirme çizgileri
    DrawLine(v[0], v[4], color); DrawLine(v[1], v[5], color);
    DrawLine(v[2], v[6], color); DrawLine(v[3], v[7], color);
}

void gizmo_system::Flush(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID)
{


    if (shaderID == 0 || m_LineVertices.empty())
    {
        std::vector<GizmoVertex>().swap(m_LineVertices);
        return;
    }

    GLenum err;
    auto checkError = [&](const std::string& label) {
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "[GIZMO FLUSH ERR AT " << label << "] " << err << std::endl;
        }
        };

    // Girişte bekleyen eski bir hata var mı temizle
    while (glGetError() != GL_NO_ERROR) {}

    // 1. Shader Bağlama
    glUseProgram(shaderID);
    checkError("glUseProgram");

    // 2. Uniform Ayarı
    int loc = glGetUniformLocation(shaderID, "u_ViewProj");
    if (loc == -1) {
        std::cout << "[GIZMO WARNING] u_ViewProj uniform lokasyonu bulunamadi! Shader'i kontrol et!" << std::endl;
    }
    glm::mat4 viewProj = projection * view;
    glUniformMatrix4fv(loc, 1, GL_FALSE, &viewProj[0][0]);
    checkError("glUniformMatrix4fv");

    // 3. State Ayarları
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glLineWidth(2.0f);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    checkError("States Setup");

    // 4. Veri Yükleme
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_LineVertices.size() * sizeof(GizmoVertex), nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_LineVertices.size() * sizeof(GizmoVertex), m_LineVertices.data());
    checkError("Buffer Data");

    // 5. Çizim
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_LineVertices.size()));
    checkError("glDrawArrays");

    // 6. Reset
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glLineWidth(1.0f);
    checkError("Reset States");

    std::vector<GizmoVertex>().swap(m_LineVertices);
}

void gizmo_system::Clear()
{
    m_LineVertices.clear();
}