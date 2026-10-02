/**
 * @file MeshRenderer.cpp
 * @brief OpenGL durum önbellekleme destekli, güvenli çizim çağrısı uygulaması.
 */

#include "MeshRenderer.h"
#include <glad/glad.h>
 // #include "../Systems/aria_system.h" // Projendeki Aria başlık yolunu buraya ekle

void MeshRenderer::Draw(const render_item& item, aria::system* ariaSystem)
{
    if (!ariaSystem) return;

    // 1. Ağ (Mesh) indis sayısını al. Çizilecek veri yoksa erken çıkış yap.
    const unsigned int indexCount = ariaSystem->get_mesh_index_count(item.model_handle, item.sub_mesh_index);
    if (indexCount == 0) return;

    // 2. Aria önbelleğinden GPU VAO kimliğini çek.
    const aria::submesh_runtime_info meshInfo = ariaSystem->get_submesh_info(item.model_handle, item.sub_mesh_index);
    const unsigned int targetVAO = meshInfo.gpu_id.vao_or_buffer_id;

    if (targetVAO == 0) return;

    // 3. VAO DURUM ÖNBELLEĞİ (STATE CACHING)
    if (m_currentVAO != targetVAO)
    {
        glBindVertexArray(targetVAO);
        m_currentVAO = targetVAO;
    }

    // 4. DOKU DURUM ÖNBELLEĞİ
    auto bindTextureIfValid = [&](const auto& handle, unsigned int slot)
        {
            if (handle.is_valid())
            {
                const unsigned int texID = ariaSystem->get_texture_id(handle);
                if (m_currentTextures[slot] != texID)
                {
                    glActiveTexture(GL_TEXTURE0 + slot);
                    glBindTexture(GL_TEXTURE_2D, texID);
                    m_currentTextures[slot] = texID;
                }
            }
        };

    bindTextureIfValid(item.diffuse_handle, RenderConstants::DIFFUSE_TEXTURE_SLOT);
    bindTextureIfValid(item.normal_handle, RenderConstants::NORMAL_TEXTURE_SLOT);
    bindTextureIfValid(item.specular_handle, RenderConstants::SPECULAR_TEXTURE_SLOT);

    // 5. ÇİZİM ÇAĞRISI (Draw Call)
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, reinterpret_cast<const void*>(0));

    // ========================================================================
    // 6. GÜVENLİK TEMİZLİĞİ (SAFE UNBIND)
    // Diğer sistemlerin (Skybox, Gizmo, UI) güncel durumu yanlış yorumlamaması 
    // için çizim sonrası bağlı kaynakları serbest bırakıyoruz.
    // ========================================================================
    if (item.diffuse_handle.is_valid()) {
        glActiveTexture(GL_TEXTURE0 + RenderConstants::DIFFUSE_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_currentTextures[RenderConstants::DIFFUSE_TEXTURE_SLOT] = 0;
    }
    if (item.normal_handle.is_valid()) {
        glActiveTexture(GL_TEXTURE0 + RenderConstants::NORMAL_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_currentTextures[RenderConstants::NORMAL_TEXTURE_SLOT] = 0;
    }
    if (item.specular_handle.is_valid()) {
        glActiveTexture(GL_TEXTURE0 + RenderConstants::SPECULAR_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_currentTextures[RenderConstants::SPECULAR_TEXTURE_SLOT] = 0;
    }

    glBindVertexArray(0);
    m_currentVAO = 0;
}

void MeshRenderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void MeshRenderer::BindFramebuffer(unsigned int fboID)
{
    if (m_currentFBO != fboID)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        m_currentFBO = fboID;
    }
}

void MeshRenderer::BindTextureSlot(unsigned int slot, unsigned int textureID)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);

    if (slot < 3)
    {
        m_currentTextures[slot] = textureID;
    }
}

void MeshRenderer::InvalidateState() noexcept
{
    m_currentVAO = 0;
    m_currentFBO = 0;
    for (int i = 0; i < 3; ++i)
    {
        m_currentTextures[i] = 0;
    }
}