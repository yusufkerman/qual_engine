#pragma once

/**
 * @file ssao_pass.h
 * @brief Ekran Uzayı Ortam Örtmesi (Screen Space Ambient Occlusion - SSAO) geçişi ve FBO yönetimi.
 */

#include <glm/glm.hpp>

class RenderTarget;
struct EngineContext;

namespace ssao_pass {

    /**
     * @brief Sahne derinlik dokusunu kullanarak SSAO, bilateral filtreleme ve harmanlama geçişlerini icra eder.
     * @param ctx Motor bağlamı (Aria, renderSettings, shader tanıtıcıları).
     * @param target Çizimin yapılacağı ve derinlik dokusunun bulunduğu hedef Framebuffer (Scene veya Game Target).
     * @param view Kamera bakış matrisi (View Matrix).
     * @param projection Kamera projeksiyon matrisi (Projection Matrix).
     * @param isSceneViewport Sahne görünümü mü (editör viewport'u mu) yoksa oyun görünümü mü?
     */
    void render(
        EngineContext& ctx,
        RenderTarget& target,
        const glm::mat4& view,
        const glm::mat4& projection,
        bool isSceneViewport
    );

    /**
     * @brief SSAO kaynaklarını (VAO, VBO, gürültü dokusu ve ara FBO'lar) serbest bırakır.
     */
    void shutdown() noexcept;

    /**
     * @brief Editör arayüzü veya debug önizlemesi için son filtrelenmiş SSAO dokusunun ID'sini döndürür.
     */
    [[nodiscard]] unsigned int get_debug_texture() noexcept;

} // namespace ssao_pass

