#pragma once

/**
 * @file shadow_map_fbo.h
 * @brief Gölge haritalama (Shadow Mapping) derinlik dokusu ve Framebuffer yöneticisi.
 */

 /**
  * @class shadow_map_fbo
  * @brief Yalnızca derinlik bileşeni (Depth Attachment) barındıran gölge tamponu sınıfı.
  */
class shadow_map_fbo
{
public:
    shadow_map_fbo() = default;
    ~shadow_map_fbo();

    // Kopyalama engellendi (OpenGL tanıtıcılarının çift serbest bırakılmasını önlemek için)
    shadow_map_fbo(const shadow_map_fbo&) = delete;
    shadow_map_fbo& operator=(const shadow_map_fbo&) = delete;

    /**
     * @brief Gölge haritası derinlik dokusunu ve FBO nesnesini tahsis eder.
     * @param width Gölge haritası çözünürlük genişliği (örn: 2048).
     * @param height Gölge haritası çözünürlük yüksekliği (örn: 2048).
     * @return true Başarıyla tahsis edildi ve FBO eksiksiz ise.
     */
    bool init(unsigned int width, unsigned int height);

    /**
     * @brief Gölge render geçişi için FBO'yu bağlar, viewport'u günceller ve derinlik tamponunu temizler.
     */
    void bind();

    /**
     * @brief Gölge çizimi tamamlandığında Framebuffer bağını çözer (0'a çeker).
     */
    void unbind();

    /**
     * @brief Ana aydınlatma geçişinde gölge hesaplaması için kullanılacak derinlik dokusu kimliğini döner.
     */
    [[nodiscard]] unsigned int get_depth_texture() const noexcept { return m_DepthTexture; }

    /**
     * @brief Kaskatli golge haritalama icin GL_TEXTURE_2D_ARRAY taniticisini doner.
     */
    [[nodiscard]] unsigned int get_depth_texture_array() const noexcept { return m_DepthTextureArray; }

    /**
     * @brief Belirtilen kaskat katmanini FBO derinlik baglantisina baglar ve temizler.
     * @param cascade_index Kaskat indeksi (0 .. m_CascadeCount - 1).
     */
    void bind_cascade(unsigned int cascade_index);

    [[nodiscard]] unsigned int get_cascade_count() const noexcept { return m_CascadeCount; }

    /**
     * @brief Spot isik golge render gecisi icin FBO'yu baglar ve derinlik tamponunu temizler.
     */
    void bind_spot();

    /**
     * @brief Noktasal isik kup golge haritasinin belirtilen yuzeyini baglar ve temizler.
     * @param face_index Kup yuzeyi indeksi (0: +X, 1: -X, 2: +Y, 3: -Y, 4: +Z, 5: -Z).
     */
    void bind_point_face(unsigned int face_index);

    [[nodiscard]] unsigned int get_spot_depth_texture() const noexcept { return m_SpotDepthTexture; }
    [[nodiscard]] unsigned int get_point_depth_cubemap() const noexcept { return m_PointDepthCubemap; }
    [[nodiscard]] unsigned int get_point_resolution() const noexcept { return m_PointResolution; }

    /**
     * @brief Gölge haritası genişliğini döner.
     */
    [[nodiscard]] unsigned int get_width() const noexcept { return m_Width; }

    /**
     * @brief Gölge haritası yüksekliğini döner.
     */
    [[nodiscard]] unsigned int get_height() const noexcept { return m_Height; }

    /**
     * @brief GPU üzerindeki FBO ve derinlik dokusu kaynaklarını serbest bırakır.
     */
    void destroy() noexcept;

private:
    unsigned int m_FBO = 0;               ///< Gölge Framebuffer nesne tanıtıcısı.
    unsigned int m_DepthTexture = 0;      ///< 32-bit GL_DEPTH_COMPONENT derinlik dokusu tanıtıcısı.
    unsigned int m_DepthTextureArray = 0; ///< GL_TEXTURE_2D_ARRAY derinlik dokusu tanıtıcısı.
    unsigned int m_CascadeCount = 3;      ///< Kaskat sayısı.
    unsigned int m_Width = 0;             ///< Gölge haritası genişliği.
    unsigned int m_Height = 0;            ///< Gölge haritası yüksekliği.

    unsigned int m_SpotFBO = 0;           ///< Spot isik Framebuffer taniticisi.
    unsigned int m_SpotDepthTexture = 0;  ///< Spot isik 2B derinlik dokusu.
    unsigned int m_PointFBO = 0;          ///< Noktasal isik Framebuffer taniticisi.
    unsigned int m_PointDepthCubemap = 0; ///< Noktasal isik kup harita (Cubemap) derinlik dokusu.
    unsigned int m_PointResolution = 1024;///< Noktasal isik kup harita cozunurlugu.
};