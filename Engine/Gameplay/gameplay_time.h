#pragma once

/**
 * @file gameplay_time.h
 * @brief Oyun betiklerinin motor zamanlama verilerine (delta_time, fixed_delta_time, time_scale, fps) erişim arayüzü.
 */

namespace gameplay {

    /**
     * @class time
     * @brief Unity Time / Unreal FApp tarzı statik zaman yönetimi sınıfı.
     */
    class time
    {
    public:
        /**
         * @brief Son kareden bu yana geçen ölçeklendirilmiş süre (saniye).
         */
        [[nodiscard]] static float delta_time();

        /**
         * @brief Donanımsal ölçeklendirilmemiş gerçek kare süresi (saniye).
         */
        [[nodiscard]] static float unscaled_delta_time();

        /**
         * @brief Fizik simülasyonu sabit zaman adımı (saniye).
         */
        [[nodiscard]] static float fixed_delta_time();

        /**
         * @brief Oyunun başladığı andan itibaren geçen toplam süre (saniye).
         */
        [[nodiscard]] static float total_time();

        /**
         * @brief Çalışma zamanı simülasyon hız katsayısı.
         */
        [[nodiscard]] static float time_scale();

        /**
         * @brief Çalışma zamanı simülasyon hız katsayısını ayarlar.
         */
        static void set_time_scale(float scale);

        /**
         * @brief Anlık ortalama kare hızı (FPS).
         */
        [[nodiscard]] static float fps();
    };

} // namespace gameplay
