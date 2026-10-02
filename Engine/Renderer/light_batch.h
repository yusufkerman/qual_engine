#pragma once

/**
 * @file light_batch.h
 * @brief Sahne içi ışık verilerinin RenderSystem'e toplu (batch) aktarılmasını sağlayan veri yapısı.
 */

#include <vector>
#include "../Systems/light_system.h"
#include "../ECS/components.h"

 /**
  * @struct light_batch
  * @brief Sahnedeki aktif ışık kaynaklarının GPU uniform aktarımı öncesinde toplandığı taşıyıcı paket.
  */
struct light_batch
{
    std::vector<light_data> lights; ///< Sahneden toplanan ışık parametreleri listesi.

    /**
     * @brief Bir sonraki kare için ışık listesini temizler.
     */
    void clear() noexcept
    {
        lights.clear();
    }
};