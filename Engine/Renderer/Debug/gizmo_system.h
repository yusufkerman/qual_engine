#pragma once

/**
 * @file gizmo_system.h
 * @brief Sahne içi görsel yardımcılar (çizgiler, sınırlayıcı kutular, küreler) için anlık mod çizim sistemi.
 */

#include <vector>
#include <glm/glm.hpp>

 /**
  * @struct GizmoVertex
  * @brief Gizmo çizgileri için 3B konum ve RGBA renk bileşenlerini barındıran tepe noktası verisi.
  */
struct GizmoVertex
{
    glm::vec3 position; ///< Çizginin uzaydaki konumu.
    glm::vec4 color;    ///< Çizginin RGBA renk değeri.
};

/**
 * @class gizmo_system
 * @brief Editör ve fizik hata ayıklama çizgilerini toplayıp tek bir çizim çağrısında (batch) GPU'ya gönderen sistem.
 */
class gizmo_system
{
public:
    gizmo_system();
    ~gizmo_system();

    /**
     * @brief OpenGL VAO ve VBO nesnelerini başlatır.
     */
    void init();

    /**
     * @brief OpenGL kaynaklarını serbest bırakır.
     */
    void shutdown();

    /**
     * @brief Başlangıç ve bitiş noktası arasında bir çizgi kaydeder.
     */
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);

    /**
     * @brief Dünya uzayında eksen hizalı tel kafes küp çizer.
     * @param center Küpün merkez konumu.
     * @param scale Küpün tam boyutu (genişlik, yükseklik, derinlik).
     * @param color Çizgi rengi.
     */
    void DrawWireCube(const glm::vec3& center, const glm::vec3& scale, const glm::vec4& color);

    /**
     * @brief Dönüşüm matrisi ve yarı boyut (half-extents) kullanarak yönlendirilmiş tel kafes kutu çizer.
     * @param transform Model dönüşüm matrisi.
     * @param extents Kutunun yarı boyutları (half-extents).
     * @param color Çizgi rengi.
     */
    void DrawWireCube(const glm::mat4& transform, const glm::vec3& extents, const glm::vec4& color);

    /**
     * @brief Dönüşüm matrisi ve yarıçap ile tel kafes küre çizer.
     * @param transform Model dönüşüm matrisi.
     * @param radius Küre yarıçapı.
     * @param color Çizgi rengi.
     */
    void DrawWireSphere(const glm::mat4& transform, float radius, const glm::vec4& color);

    /**
     * @brief Kayıtlı tüm gizmo çizgilerini GPU'ya yükler ve ekrana çizer.
     * @param view Kameranın görünüm (View) matrisi.
     * @param projection Kameranın projeksiyon (Projection) matrisi.
     * @param shaderID Gizmo çiziminde kullanılacak shader programı kimliği.
     */
    void Flush(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID);

    /**
     * @brief Çizim listesindeki tüm çizgileri temizler.
     */
    void Clear();

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    size_t m_AllocatedCapacity = 0;

    std::vector<GizmoVertex> m_LineVertices;
};