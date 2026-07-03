// MeshRenderer.cpp
#include "MeshRenderer.h"
#include "../Engine/engine_context.h" // Aria erişimi için
#include <glad/glad.h>
#include <iostream>

/**
 * @brief Aria sistemini kullanarak render işlemini gerçekleştirir.
 * Eski statik vektör yapısı ve texture_manager bağımlılığı kaldırılmıştır.
 */
void MeshRenderer::Draw(const render_item& item, aria::system* ariaSystem)
{
    if (!ariaSystem) return;

    GLint currentVAO;
    GLint currentProgram;

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    

    // 🚀 1. MESH PAKETİNİ AL VE UNPACK ET
    // Aria bize VAO, VBO ve EBO bilgilerini bir paket (struct) olarak veriyor.
    // Biz buradan sadece VAO (vao_or_buffer_id) bilgisini çekiyoruz.
    aria::submesh_runtime_info meshInfo =
        ariaSystem->get_submesh_info(item.model_handle, item.sub_mesh_index);

    glBindVertexArray(meshInfo.gpu_id.vao_or_buffer_id);

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

    // 🚀 2. DOKULARI ARIA ÜZERİNDEN BAĞLA
    // texture_manager yerine doğrudan ariaSystem üzerinden doku ID'lerini çekiyoruz.
    if (item.diffuse_handle.is_valid())
    {
        unsigned int diffuseGLid = ariaSystem->get_texture_id(item.diffuse_handle);
        glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, diffuseGLid);
    }

    if (item.normal_handle.is_valid())
    {
        unsigned int normalGLid = ariaSystem->get_texture_id(item.normal_handle);
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, normalGLid);
    }

    if (item.specular_handle.is_valid())
    {
        unsigned int specularGLid = ariaSystem->get_texture_id(item.specular_handle);
        glActiveTexture(GL_TEXTURE0 + SPECULAR_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, specularGLid);
    }

    // 🚀 3. ÇİZİM EMRİ
    // Aria'dan gelen submesh'e ait indeks sayısını kullanıyoruz.
    // (Aria sisteminde get_mesh_index_count gibi bir metodun olduğunu varsayıyorum)
    unsigned int indexCount = ariaSystem->get_mesh_index_count(item.model_handle, item.sub_mesh_index);

    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

    // 🚀 4. STATE SIZINTI KORUMASI (UNBIND)
    // Render state'lerinin bir sonraki çizimi bozmaması için temizliyoruz.
    if (item.diffuse_handle.is_valid()) {
        glActiveTexture(GL_TEXTURE0 + DIFFUSE_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (item.normal_handle.is_valid()) {
        glActiveTexture(GL_TEXTURE0 + NORMAL_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (item.specular_handle.is_valid()) {
        glActiveTexture(GL_TEXTURE0 + SPECULAR_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }


    glBindVertexArray(0);

}

// 🛠️ YARDIMCI FONKSİYONLAR (Viewport ve Framebuffer yönetimi aynen devam)
void MeshRenderer::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void MeshRenderer::BindFramebuffer(unsigned int fboID)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
}

void MeshRenderer::BindTextureSlot(unsigned int slot, unsigned int textureID)
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
}