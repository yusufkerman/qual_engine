#pragma once

/**
 * @file render_queue.h
 * @brief Çizim emirlerini durum değişikliklerini en aza indirmek için gruplayan render kuyruğu.
 */

#include <vector>
#include <unordered_map>
#include "render_item.h"

/**
 * @struct render_queue
 * @brief Shader kimliğine göre gruplanmış (batched) çizim emirlerini tutan veri yapısı.
 */
struct render_queue
{
    /// Shader kimliğine göre paketlenmiş çizim emirleri tablosu.
    std::unordered_map<unsigned int, std::vector<render_item>> batches;

    /**
     * @brief Vektör kapasitelerini serbest bırakmadan (reallocation önleyerek) kuyruğu bir sonraki kare için temizler.
     */
    void clear() noexcept
    {
        for (auto& [shaderID, items] : batches)
        {
            items.clear();
        }
    }

    /**
     * @brief Sahne değişimlerinde veya motor kapanışında tüm kuyruğu tamamen boşaltır.
     */
    void reset() noexcept
    {
        batches.clear();
    }
};