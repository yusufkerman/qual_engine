#pragma once

/**
 * @file gizmo_system.h
 * @brief Sahne içi görsel yardımcılar (çizgiler, sınırlayıcı kutular, küreler, kameralar, ışıklar) için modüler anlık mod çizim sistemi.
 */

#include <vector>
#include <glm/glm.hpp>
#include "gizmo_settings.h"

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
 * @brief Editör, fizik ve oynanış (Gameplay) hata ayıklama çizgilerini toplayıp tek bir çizim çağrısında (batch) GPU'ya gönderen modüler sistem.
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

    // ========================================================================
    // AYARLAR (SETTINGS) ERİŞİMİ
    // ========================================================================

    [[nodiscard]] gizmo_settings& settings() noexcept { return m_Settings; }
    [[nodiscard]] const gizmo_settings& settings() const noexcept { return m_Settings; }

    // ========================================================================
    // TEMEL GEOMETRİK PRİMİTİFLER (EDITOR & GENEL GİZMO KATMANI)
    // ========================================================================

    /**
     * @brief Başlangıç ve bitiş noktası arasında bir çizgi kaydeder.
     */
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);

    /**
     * @brief Başlangıç noktasından verilen yön+uzunluk vektörü boyunca ışın çizer.
     */
    void DrawRay(const glm::vec3& origin, const glm::vec3& direction, const glm::vec4& color);

    /**
     * @brief Başlangıçtan bitişe doğru ok başlı yönlü bir vektör çizer.
     */
    void DrawArrow(const glm::vec3& start, const glm::vec3& end, float headSize, const glm::vec4& color);

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
     * @brief Merkez noktası ve yarıçap ile 3 eksenli tel kafes küre çizer.
     */
    void DrawWireSphere(const glm::vec3& center, float radius, const glm::vec4& color);

    /**
     * @brief Verilen merkez ve normal vektörüne dik düzlemde tel kafes çember çizer.
     */
    void DrawWireCircle(
        const glm::vec3& center,
        const glm::vec3& normal,
        float radius,
        const glm::vec4& color,
        int segments = 40);

    /**
     * @brief Verilen düzlemde belirli bir açı aralığında çember yayı (arc) çizer.
     */
    void DrawWireArc(
        const glm::vec3& center,
        const glm::vec3& normal,
        const glm::vec3& startDir,
        float angleRad,
        float radius,
        const glm::vec4& color,
        int segments = 24);

    /**
     * @brief Tepe noktasından hedef yöne açılan tel kafes koni (Spot Light vb.) çizer.
     */
    void DrawWireCone(
        const glm::vec3& apex,
        const glm::vec3& direction,
        float range,
        float halfAngleDeg,
        const glm::vec4& color,
        int segments = 36);

    /**
     * @brief Perspektif kamera görüş piramidini (Frustum) tel kafes olarak çizer.
     */
    void DrawCameraFrustum(
        const glm::vec3& position,
        const glm::vec3& forward,
        const glm::vec3& right,
        const glm::vec3& up,
        float fovDeg,
        float aspect,
        float nearPlane,
        float farPlane,
        const glm::vec4& color);

    /**
     * @brief Yönlü ışık (Directional Light) için güneş diski ve paralel ışık okları çizer.
     */
    void DrawDirectionalLightGizmo(
        const glm::vec3& position,
        const glm::vec3& direction,
        float radius,
        float rayLength,
        const glm::vec4& color);

    // ========================================================================
    // GAMEPLAY API (ÇALIŞMA ZAMANI HATA AYIKLAMA) KATMANI
    // ========================================================================

    /**
     * @brief Gameplay betiklerinin çizdiği çalışma zamanı gizmo tamponunu etkinleştirir/devre dışı bırakır.
     */
    void SetGameplayWriteMode(bool gameplayMode) noexcept { m_WritingGameplay = gameplayMode; }

    /**
     * @brief Sadece çalışma zamanı (Gameplay) gizmo çizgilerini temizler.
     */
    void ClearGameplay();

    /**
     * @brief Çalışma zamanı (Gameplay) gizmo çizgilerini GPU'ya gönderir (Tamponu silmez).
     */
    void RenderGameplayGizmos(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID);

    // ========================================================================
    // FLUSH & CLEAR
    // ========================================================================

    /**
     * @brief Kayıtlı tüm editör gizmo çizgilerini GPU'ya yükler ve ekrana çizer.
     * @param view Kameranın görünüm (View) matrisi.
     * @param projection Kameranın projeksiyon (Projection) matrisi.
     * @param shaderID Gizmo çiziminde kullanılacak shader programı kimliği.
     */
    void Flush(const glm::mat4& view, const glm::mat4& projection, unsigned int shaderID);

    /**
     * @brief Editör çizim listesindeki tüm çizgileri temizler.
     */
    void Clear();

private:
    void DrawVertexBatch(
        const std::vector<GizmoVertex>& vertices,
        const glm::mat4& view,
        const glm::mat4& projection,
        unsigned int shaderID,
        float lineWidth);

private:
    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    size_t m_AllocatedCapacity = 0;

    bool m_WritingGameplay = false;
    gizmo_settings m_Settings;

    std::vector<GizmoVertex> m_LineVertices;
    std::vector<GizmoVertex> m_GameplayLineVertices;
};