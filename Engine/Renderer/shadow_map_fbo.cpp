/**
 * @file shadow_map_fbo.cpp
 * @brief Derinlik tamponu tahsisi, sınır rengi yapılandırması ve FBO yaşam döngüsü uygulaması.
 */

#include "shadow_map_fbo.h"
#include <glad/glad.h>
#include <iostream>

shadow_map_fbo::~shadow_map_fbo()
{
    destroy();
}

bool shadow_map_fbo::init(unsigned int width, unsigned int height)
{
    // Yeniden başlatma durumunda eski kaynakları serbest bırak
    destroy();

    m_Width = width;
    m_Height = height;

    // 1. Framebuffer oluştur
    glGenFramebuffers(1, &m_FBO);

    // 2. Yalnızca derinlik verisi tutacak olan dokuyu yapılandır
    glGenTextures(1, &m_DepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        static_cast<GLsizei>(m_Width),
        static_cast<GLsizei>(m_Height),
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    // Gölge filtreleme parametreleri
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Işık frustum'ı dışındaki koordinatların gölgede kalmaması için sınır rengini 1.0 (beyaz/uzak) yap
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // 3. Dokuyu Framebuffer'ın derinlik noktasına bağla
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

    // Renk tamponu yazım ve okuma işlemlerini devre dışı bırak
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Doğrulama kontrolü
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "[ShadowMapFBO] Hata: Framebuffer eksik veya gecersiz! (Durum: " << status << ")\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        destroy();
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void shadow_map_fbo::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height));
    glClear(GL_DEPTH_BUFFER_BIT);
}

void shadow_map_fbo::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void shadow_map_fbo::destroy() noexcept
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

    m_Width = 0;
    m_Height = 0;
}