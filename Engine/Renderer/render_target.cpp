/**
 * @file render_target.cpp
 * @brief Framebuffer, renk dokusu ve derinlik tamponu oluşturma, yeniden boyutlandırma ve temizleme uygulaması.
 */

#include "render_target.h"
#include <iostream>
#include <algorithm>

namespace {

    /**
     * @brief Boyut değerinin en az 1 piksel olmasını garanti eder.
     */
    inline int clampSize(int v) noexcept
    {
        return std::max(1, v);
    }

} // anonymous namespace

void RenderTarget::create(int w, int h)
{
    width = clampSize(w);
    height = clampSize(h);

    if (fbo == 0) {
        glGenFramebuffers(1, &fbo);
    }

    if (tex == 0) {
        glGenTextures(1, &tex);
    }

    if (rbo == 0) {
        glGenRenderbuffers(1, &rbo);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Renk eki olarak kullanılacak 2B dokuyu yapılandır
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    // Derinlik ve şablon (Depth/Stencil) tamponunu yapılandır
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "[RenderTarget] Framebuffer tamamlanamadi (Hata: " << status << ")\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::resize(int w, int h)
{
    w = clampSize(w);
    h = clampSize(h);

    if (w == width && h == height) {
        return;
    }

    width = w;
    height = h;

    // Doku belleğini yeni boyutla yeniden tahsis et
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    // Renderbuffer belleğini yeni boyutla yeniden tahsis et
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}

void RenderTarget::destroy()
{
    if (rbo != 0)
    {
        glDeleteRenderbuffers(1, &rbo);
        rbo = 0;
    }

    if (tex != 0)
    {
        glDeleteTextures(1, &tex);
        tex = 0;
    }

    if (fbo != 0)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
}