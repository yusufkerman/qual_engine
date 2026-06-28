#include "shadow_map_fbo.h"
#include <glad/glad.h>
#include <iostream>

shadow_map_fbo::shadow_map_fbo() {}

shadow_map_fbo::~shadow_map_fbo()
{
    destroy();
}

bool shadow_map_fbo::init(unsigned int width, unsigned int height)
{
    m_Width = width;
    m_Height = height;

    // 1. Framebuffer oluştur
    glGenFramebuffers(1, &m_FBO);

    // 2. Sadece derinlik tutacak olan dokuyu (Texture) oluştur
    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);

    // Dokunun formatını GL_DEPTH_COMPONENT yapıyoruz (Renk yok, sadece 32-bit derinlik)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Gölge haritalarında kenar piksellerin taşmasını önlemek için filtreleme ayarları
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Işık alanının dışındaki yerlerin gölgede kalmaması için kenarları beyaz (gölgesiz) kabul et
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 3. Dokuyu Framebuffer'ın DEPTH attachment noktasına bağla
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

    // OpenGL'e bu framebuffer'a RENK VERİSİ YAZMAYACAĞIMIZI açıkça söylüyoruz (Performans için kritik)
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Framebuffer düzgün oluştu mu kontrol et
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "[ShadowMapFBO] Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void shadow_map_fbo::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    // Ekran boyutunu değil, gölge haritasının kendi çözünürlüğünü (örn: 2048x2048) set ediyoruz
    glViewport(0, 0, m_Width, m_Height);
    // Sadece derinlik tamponunu temizliyoruz, renk tamponu zaten yok
    glClear(GL_DEPTH_BUFFER_BIT);
}

void shadow_map_fbo::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void shadow_map_fbo::destroy()
{
    if (m_DepthTexture != 0)
    {
        glDeleteTextures(1, &m_DepthTexture);
        m_DepthTexture = 0;
    }
    if (m_FBO != 0)
    {
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
    }
}