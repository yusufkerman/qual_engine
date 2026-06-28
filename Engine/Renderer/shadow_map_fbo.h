#pragma once

class shadow_map_fbo
{
public:
    shadow_map_fbo();
    ~shadow_map_fbo();

    // Gölge haritası dokusunu ve FBO'yu ayağa kaldırır
    bool init(unsigned int width, unsigned int height);

    // Gölge pass'ine başlarken bu FBO'yu bağlar
    void bind();

    // Gölge pass'i bittiğinde normal ekrana dönmek için çözülür
    void unbind();

    // Ana shader'da gölgeyi okumak için texture ID'sini döner
    unsigned int get_depth_texture() const { return m_DepthTexture; }
    unsigned int get_width() const { return m_Width; }
    unsigned int get_height() const { return m_Height; }

    void destroy();

private:
    unsigned int m_FBO = 0;
    unsigned int m_DepthTexture = 0;
    unsigned int m_Width = 0;
    unsigned int m_Height = 0;
};