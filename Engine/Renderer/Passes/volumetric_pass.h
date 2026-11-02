#pragma once

/**
 * @file volumetric_pass.h
 * @brief Hacimsel ışıklandırma (Volumetric Lighting / Crepuscular God Rays) geçişi.
 *
 * Yönlü ışık (Directional Light) ve kaskatlı gölge haritaları (CSM) ile entegre ışın yürütme
 * (raymarching) gerçekleştirerek ışık süzmelerini hesaplar ve sahne framebuffer'ına
 * eklemeli (additive) olarak uygular. Hem varsayılan PBR boru hattını hem de PS1 Retro
 * Horror boru hattını (4x4 Bayer dithering ve RGB555 kuantizasyonu) destekler.
 */

#include <glm/glm.hpp>

struct EngineContext;
class RenderTarget;

namespace volumetric_pass {

    /**
     * @brief Hacimsel ışıklandırma geçişini yürütür.
     *
     * @param ctx Motor çalışma zamanı bağlamı.
     * @param target Çizimin yapılacağı ana render hedefi (gameTarget veya editorTarget).
     * @param view Kameranın görünüm (View) matrisi.
     * @param projection Kameranın iz düşüm (Projection) matrisi.
     * @param cameraPosition Kameranın dünya uzayındaki konumu.
     * @param isSceneViewport Çizimin sahne görünümü (Scene View) için mi yoksa oyun (Game View) için mi yapıldığı.
     */
    void render(
        EngineContext& ctx,
        RenderTarget& target,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        bool isSceneViewport
    );

    /**
     * @brief Hacimsel ışıklandırma geçişine ait GPU kaynaklarını (FBO, VAO, VBO) serbest bırakır.
     */
    void shutdown() noexcept;

} // namespace volumetric_pass

