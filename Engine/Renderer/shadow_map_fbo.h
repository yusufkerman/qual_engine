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
    unsigned int m_FBO = 0;          ///< Gölge Framebuffer nesne tanıtıcısı.
    unsigned int m_DepthTexture = 0; ///< 32-bit GL_DEPTH_COMPONENT derinlik dokusu tanıtıcısı.
    unsigned int m_Width = 0;        ///< Gölge haritası genişliği.
    unsigned int m_Height = 0;       ///< Gölge haritası yüksekliği.
};