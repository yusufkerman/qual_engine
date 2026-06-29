#pragma once
#include <vector>
#include <glm/glm.hpp>

struct GizmoVertex
{
    glm::vec3 position;
    glm::vec4 color;
};

class gizmo_system
{
public:
    gizmo_system();
    ~gizmo_system();

    void init();
    void shutdown();

    // Anlık çizim emirleri (Immediate Mode API)
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
    void DrawWireCube(const glm::vec3& center, const glm::vec3& scale, const glm::vec4& color);
    void DrawWireCube(const glm::mat4& transform, const glm::vec3& extents, const glm::vec4& color);
    void DrawWireSphere(const glm::mat4& transform, float radius, const glm::vec4& color);

    // Frame sonunda buffer'ları boşaltıp ekrana çizen ana fonksiyon
    void Flush(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID);
    void Clear();

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;

    std::vector<GizmoVertex> m_LineVertices;
};