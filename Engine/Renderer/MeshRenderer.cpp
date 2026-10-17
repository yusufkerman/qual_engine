/**
 * @file MeshRenderer.cpp
 * @brief OpenGL durum önbellekleme destekli, güvenli çizim çağrısı uygulaması.
 */

#include "MeshRenderer.h"
#include <glad/glad.h>
#include <algorithm>
#include <vector>

MeshRenderer::~MeshRenderer()
{
    if (m_dynamicBoneUBO != 0)
    {
        glDeleteBuffers(1, &m_dynamicBoneUBO);
        m_dynamicBoneUBO = 0;
    }
}

void MeshRenderer::ensure_bone_ubo_allocated()
{
    if (m_dynamicBoneUBO == 0)
    {
        glGenBuffers(1, &m_dynamicBoneUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_dynamicBoneUBO);

        // 128 adet birim matris (Identity) ile ilklendir
        std::vector<glm::mat4> identityMatrices(aria::MAX_BONES_PER_SKELETON, glm::mat4(1.0f));
        glBufferData(GL_UNIFORM_BUFFER, identityMatrices.size() * sizeof(glm::mat4), identityMatrices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

unsigned int MeshRenderer::Draw(const render_item& item, aria::system* ariaSystem)
{
    if (!ariaSystem) return 0;

    unsigned int indexCount = 0;
    unsigned int targetVAO = 0;

    // 1. Statik vs İskeletli (Skinned) Mesh Ayrımı
    if (item.is_skinned)
    {
        if (!item.skinned_model_handle.is_valid()) return 0;

        indexCount = static_cast<unsigned int>(ariaSystem->get_skinned_mesh_index_count(item.skinned_model_handle, item.sub_mesh_index));
        if (indexCount == 0) return 0;

        const aria::submesh_runtime_info meshInfo = ariaSystem->get_skinned_submesh_info(item.skinned_model_handle, item.sub_mesh_index);
        targetVAO = meshInfo.gpu_id.vao_or_buffer_id;

        // 2. Kemik Matrislerini UBO ile GPU'ya İlet
        if (item.bone_ubo != 0)
        {
            glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstants::BONE_UBO_BINDING_SLOT, item.bone_ubo);
        }
        else if (item.bone_matrices != nullptr && !item.bone_matrices->empty())
        {
            ensure_bone_ubo_allocated();
            size_t uploadCount = std::min(item.bone_matrices->size(), static_cast<size_t>(aria::MAX_BONES_PER_SKELETON));

            glBindBuffer(GL_UNIFORM_BUFFER, m_dynamicBoneUBO);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, uploadCount * sizeof(glm::mat4), item.bone_matrices->data());
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstants::BONE_UBO_BINDING_SLOT, m_dynamicBoneUBO);
        }
        else
        {
            // Eğer kemik matrisi atanmamışsa bind pose (Birim matrisler) ile güvenle çiz
            ensure_bone_ubo_allocated();
            glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstants::BONE_UBO_BINDING_SLOT, m_dynamicBoneUBO);
        }
    }
    else
    {
        if (!item.model_handle.is_valid()) return 0;

        indexCount = static_cast<unsigned int>(ariaSystem->get_mesh_index_count(item.model_handle, item.sub_mesh_index));
        if (indexCount == 0) return 0;

        const aria::submesh_runtime_info meshInfo = ariaSystem->get_submesh_info(item.model_handle, item.sub_mesh_index);
        targetVAO = meshInfo.gpu_id.vao_or_buffer_id;
    }

    if (targetVAO == 0) return 0;

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

    if (item.is_skinned)
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstants::BONE_UBO_BINDING_SLOT, 0);
    }

    glBindVertexArray(0);
    m_currentVAO = 0;

    return indexCount;
}

void MeshRenderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void MeshRenderer::BindFramebuffer(unsigned int fboID)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    m_currentFBO = fboID;
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