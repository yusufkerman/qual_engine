#pragma once

/**
 * @file skybox_pass.h
 * @brief Sahne arka planı küp haritası (Skybox Cubemap) render geçişini tanımlar.
 */

#include <glm/glm.hpp>

struct EngineContext;

namespace skybox_pass
{
    /**
     * @brief Gökyüzü küp haritasını kamera yönelimine göre sonsuz uzaklıkta çizer.
     *
     * @param ctx Motor bağlamı, cubemap dokusu ve shader tanıtıcıları.
     * @param view Kameranın görünüm (View) matrisi.
     * @param projection Kameranın projeksiyon (Projection) matrisi.
     */
    void render(EngineContext& ctx, const glm::mat4& view, const glm::mat4& projection);

    /**
     * @brief Skybox küp geometrisi için ayrılmış OpenGL VAO ve VBO tamponlarını serbest bırakır.
     */
    void shutdown();
}