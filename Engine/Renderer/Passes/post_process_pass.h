#pragma once

/**
 * @file post_process_pass.h
 * @brief HDR Tone Mapping (None, Reinhard, ACES, AgX, Uncharted 2), sRGB Gamma ve Anti-Aliasing (FXAA 3.11 & TAA) geçişi.
 */

#include <glm/glm.hpp>

struct EngineContext;
class RenderTarget;

namespace post_process_pass
{
    /**
     * @brief TAA aktifken kamera projeksiyon matrisine Halton (2, 3) alt-piksel titremesi (jitter) uygular.
     * TAA kapalıyken matrisi birebir değiştirmeden döndürür.
     */
    glm::mat4 apply_taa_jitter(
        const EngineContext& ctx,
        const glm::mat4& unjitteredProj,
        int width,
        int height,
        bool isSceneViewport
    ) noexcept;

    /**
     * @brief HDR sahne tamponu üzerinde TAA zaman içi birleştirme, Tone Mapping, Gamma düzeltmesi ve FXAA/Sharpen uygular.
     */
    void render(
        EngineContext& ctx,
        RenderTarget& target,
        const glm::mat4& view,
        const glm::mat4& unjitteredProj,
        bool isSceneViewport
    );

    /**
     * @brief Post-process FBO ve geçmiş (history) tamponlarını serbest bırakır.
     */
    void shutdown() noexcept;
}
