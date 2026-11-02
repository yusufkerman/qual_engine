#pragma once

/**
 * @file render_target.h
 * @brief Sahne ve arayüz çizimleri için Framebuffer (FBO), renk dokusu ve derinlik tamponunu yöneten sınıf.
 */

#include <glad/glad.h>

 /**
  * @class RenderTarget
  * @brief Ekran dışı (off-screen) çizim hedeflerinin (FBO, doku, derinlik dokusu) yaşam döngüsünü yönetir.
  */
class RenderTarget
{
public:
    unsigned int fbo = 0;       ///< OpenGL Framebuffer nesne tanıtıcısı.
    unsigned int tex = 0;       ///< FBO renk eklentisine bağlı 2B doku tanıtıcısı.
    unsigned int depth_tex = 0; ///< FBO derinlik ve şablon eklentisine bağlı 2B doku tanıtıcısı (Shader'lardan sampler2D olarak okunabilir).
    unsigned int rbo = 0;       ///< Geriye dönük uyumluluk için renderbuffer tanıtıcısı.

    int width = 1;          ///< Hedef tampon piksel genişliği.
    int height = 1;         ///< Hedef tampon piksel yüksekliği.

    RenderTarget() = default;
    ~RenderTarget() { destroy(); }

    /**
     * @brief Belirtilen çözünürlükte FBO, doku ve derinlik tampon bileşenlerini tahsis eder.
     * @param w Genişlik (piksel cinsinden, minimum 1).
     * @param h Yükseklik (piksel cinsinden, minimum 1).
     */
    void create(int w, int h);

    /**
     * @brief Var olan doku ve derinlik tamponu boyutlarını yeni çözünürlüğe göre günceller.
     * @param w Yeni genişlik.
     * @param h Yeni yükseklik.
     */
    void resize(int w, int h);

    /**
     * @brief GPU üzerindeki FBO, renk dokusu ve derinlik dokusu kaynaklarını serbest bırakır.
     */
    void destroy();
};
