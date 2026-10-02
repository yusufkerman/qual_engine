#pragma once

/**
 * @file MeshRenderer.h
 * @brief Aria varlık sistemiyle entegre, geriye dönük uyumlu ve önbellekli ağ (mesh) çizici.
 */

#include "../Renderer/render_item.h"

 // Aria sistemine erişebilmek için ileri bildirim (forward declaration)
namespace aria { class system; }

/**
 * @enum RenderConstants
 * @brief Grafik boru hattı (pipeline) sabitleri ve doku yuvaları.
 */
enum RenderConstants : int
{
    MAX_LIGHT_COUNT = 64,       ///< Sahnede desteklenen maksimum aktif ışık sayısı.
    DIFFUSE_TEXTURE_SLOT = 0,   ///< Albedo/Diffuse dokusunun bağlanacağı OpenGL yuvası.
    NORMAL_TEXTURE_SLOT = 1,    ///< Normal haritasının bağlanacağı OpenGL yuvası.
    SPECULAR_TEXTURE_SLOT = 2   ///< Speküler/Parlaklık haritasının bağlanacağı OpenGL yuvası.
};

/**
 * @class MeshRenderer
 * @brief Alt seviye OpenGL durum yönetimini ve çizim komutlarını icra eden sınıf.
 *
 * @note Bu sürüm, `glGetIntegerv` darboğazlarını çözerken, diğer render geçişlerini
 * kırmamak adına çizim sonrasında durumu güvenli bir şekilde sıfırlayacak (unbind) şekilde tasarlanmıştır.
 */
class MeshRenderer
{
public:
    MeshRenderer() = default;
    ~MeshRenderer() = default;

    /**
     * @brief Belirtilen render öğesini Aria varlık sisteminden çözerek ekrana çizer.
     *
     * @param item Çizilecek alt ağ (sub-mesh) ve materyal doku tanıtıcıları.
     * @param ariaSystem GPU kaynak önbelleğine erişim sağlayan Aria ana sistemi.
     */
    void Draw(const render_item& item, aria::system* ariaSystem);

    /**
     * @brief Render hedefi görünüm alanını (Viewport) ayarlar.
     */
    void SetViewport(int x, int y, int width, int height);

    /**
     * @brief Belirtilen Framebuffer nesnesini aktif hedefe bağlar.
     * @param fboID Bağlanacak Framebuffer nesnesinin kimliği.
     */
    void BindFramebuffer(unsigned int fboID);

    /**
     * @brief Belirtilen doku birimine (Texture Unit) bir doku bağlar.
     * @param slot Dokunun bağlanacağı yuva (Örn: GL_TEXTURE0 + slot).
     * @param textureID Bağlanacak OpenGL doku kimliği.
     */
    void BindTextureSlot(unsigned int slot, unsigned int textureID);

    /**
     * @brief Harici bir sistem (ör. ImGui, Gizmo) OpenGL durumunu değiştirirse önbelleği sıfırlamak için kullanılır.
     * @note Her ana render döngüsünün (Pass) başında çağrılmalıdır.
     */
    void InvalidateState() noexcept;

private:
    unsigned int m_currentVAO = 0;                  ///< Şu an GPU'da aktif olan VAO kimliği.
    unsigned int m_currentFBO = 0;                  ///< Şu an GPU'da aktif olan FBO kimliği.
    unsigned int m_currentTextures[3]{ 0, 0, 0 };   ///< Yuvalara bağlı olan aktif doku kimlikleri.
};