#pragma once

/**
 * @file frustum.h
 * @brief Kamera görüş piramidi (Frustum) düzlemlerini ve görünürlük kırpma (Frustum Culling) testlerini tanımlar.
 */

#include <glm/glm.hpp>
#include <array>

 /**
  * @struct plane
  * @brief 3B uzayda ax + by + cz + d = 0 denklemiyle tanımlanan geometrik düzlem yapısı.
  */
struct plane
{
    glm::vec3 normal{ 0.0f, 1.0f, 0.0f }; ///< Düzlemin dışarıya bakan birim normal vektörü.
    float d = 0.0f;                       ///< Düzlemin orijine olan işaretli mesafesi.

    /**
     * @brief Verilen bir noktanın düzleme olan işaretli mesafesini hesaplar.
     * @param p Test edilecek 3B nokta.
     * @return float Pozitif ise düzlemin önünde, negatif ise arkasında.
     */
    [[nodiscard]] float distance(const glm::vec3& p) const noexcept
    {
        return glm::dot(normal, p) + d;
    }
};

/**
 * @struct frustum
 * @brief Kameranın 6 görüş düzlemini (Sol, Sağ, Alt, Üst, Yakın, Uzak) barındıran piramit hacmi.
 */
struct frustum
{
    std::array<plane, 6> planes; ///< Sol (0), Sağ (1), Alt (2), Üst (3), Yakın (4), Uzak (5) düzlemleri.

    /**
     * @brief Verilen çevreleyen kürenin (Bounding Sphere) görüş hacmi içinde olup olmadığını test eder.
     *
     * @param worldCenter Kürenin dünya uzayındaki merkez konumu.
     * @param radius Kürenin dünya uzayındaki yarıçapı.
     * @return true Küre görüş hacmi içindeyse veya düzlemleri kesiyorsa (görünür).
     * @return false Küre en az bir düzlemin tamamen arkasında kalıyorsa (kırpılır/görünmez).
     */
    [[nodiscard]] bool is_inside(const glm::vec3& worldCenter, float radius) const noexcept
    {
        for (const auto& p : planes)
        {
            if (p.distance(worldCenter) < -radius)
            {
                return false;
            }
        }
        return true;
    }
};

/**
 * @brief Görünüm-Projeksiyon (View-Projection) matrisinden normalize edilmiş 6 adet frustum düzlemini çıkarır (Gribb-Hartmann).
 *
 * @param m Birleşik Görünüm * Projeksiyon matrisi.
 * @return frustum Çıkarılan ve normalize edilen 6 görüş düzlemi.
 */
inline frustum build_frustum(const glm::mat4& m) noexcept
{
    frustum f;

    // SOL (LEFT) DÜZLEM
    f.planes[0].normal = glm::vec3(
        m[0][3] + m[0][0],
        m[1][3] + m[1][0],
        m[2][3] + m[2][0]
    );
    f.planes[0].d = m[3][3] + m[3][0];

    // SAĞ (RIGHT) DÜZLEM
    f.planes[1].normal = glm::vec3(
        m[0][3] - m[0][0],
        m[1][3] - m[1][0],
        m[2][3] - m[2][0]
    );
    f.planes[1].d = m[3][3] - m[3][0];

    // ALT (BOTTOM) DÜZLEM
    f.planes[2].normal = glm::vec3(
        m[0][3] + m[0][1],
        m[1][3] + m[1][1],
        m[2][3] + m[2][1]
    );
    f.planes[2].d = m[3][3] + m[3][1];

    // ÜST (TOP) DÜZLEM
    f.planes[3].normal = glm::vec3(
        m[0][3] - m[0][1],
        m[1][3] - m[1][1],
        m[2][3] - m[2][1]
    );
    f.planes[3].d = m[3][3] - m[3][1];

    // YAKIN (NEAR) DÜZLEM
    f.planes[4].normal = glm::vec3(
        m[0][3] + m[0][2],
        m[1][3] + m[1][2],
        m[2][3] + m[2][2]
    );
    f.planes[4].d = m[3][3] + m[3][2];

    // UZAK (FAR) DÜZLEM
    f.planes[5].normal = glm::vec3(
        m[0][3] - m[0][2],
        m[1][3] - m[1][2],
        m[2][3] - m[2][2]
    );
    f.planes[5].d = m[3][3] - m[3][2];

    // Düzlem normallerini normalize et
    for (auto& p : f.planes)
    {
        const float len = glm::length(p.normal);
        if (len > 0.0001f)
        {
            p.normal /= len;
            p.d /= len;
        }
    }

    return f;
}

/**
 * @brief Geriye dönük uyumluluk için camelCase adlandırma sarmalayıcısı.
 */
inline frustum buildFrustum(const glm::mat4& m) noexcept
{
    return build_frustum(m);
}