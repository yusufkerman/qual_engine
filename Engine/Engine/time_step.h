#pragma once

/**
 * @file time_step.h
 * @brief Yüksek hassasiyetli zaman ölçümü, dinamik kare süresi (delta time) ve
 * Glenn Fiedler'in "Fix Your Timestep" (Accumulator) algoritmasını yöneten zamanlama sınıfı.
 */

#include <chrono>
#include <cstdint>

namespace core {

    /**
     * @class time_step
     * @brief Monotonik saat (std::chrono::steady_clock) kullanarak değişken render kareleri ile
     * sabit frekanslı (ör. 60 Hz) fizik simülasyonunu birbirine bağlayan zamanlayıcı motoru.
     */
    class time_step
    {
    public:
        time_step();

        /**
         * @brief Her grafik karesinin başında çağrılır ve son kareden bu yana geçen süreyi hesaplar.
         */
        void tick();

        /**
         * @brief Birikmiş süreden (accumulator) bir sabit fizik adımı tüketir.
         * Sabit adım süresi kadar zaman birikmişse true döner ve süreyi düşer.
         *
         * @return true Eğer bir fizik simülasyon adımı çalıştırılması gerekiyorsa.
         */
        bool consume_fixed_step();

        /**
         * @brief Mod geçişlerinde veya duraklatma durumlarında birikmiş süreyi sıfırlar.
         * (Uzun duraksamalar sonrası fizik dünyasında patlama veya sıçrama olmasını önler).
         */
        void reset_accumulator();

        // ==========================================
        // ERİŞİM VE YAPILANDIRMA (GETTERS & SETTERS)
        // ==========================================

        /**
         * @brief Zaman ölçeği uygulanmış dinamik kare süresi (saniye).
         */
        [[nodiscard]] float delta_time() const noexcept { return m_delta_time; }

        /**
         * @brief Zaman ölçeğinden bağımsız saf donanım kare süresi (saniye).
         */
        [[nodiscard]] float unscaled_delta_time() const noexcept { return m_unscaled_delta_time; }

        /**
         * @brief Fizik simülasyonunun sabit adım süresi (varsayılan: 1/60 = ~0.01667 sn).
         */
        [[nodiscard]] float fixed_delta_time() const noexcept { return m_fixed_delta_time; }

        /**
         * @brief Motorun başlatılmasından bu yana geçen toplam süre (saniye).
         */
        [[nodiscard]] float total_time() const noexcept { return m_total_time; }

        /**
         * @brief Zaman ölçeği katsayısı (1.0 = normal hız, 0.5 = ağır çekim, 2.0 = hızlı).
         */
        [[nodiscard]] float time_scale() const noexcept { return m_time_scale; }

        /**
         * @brief Başlangıçtan itibaren işlenen toplam render karesi sayısı.
         */
        [[nodiscard]] uint64_t frame_count() const noexcept { return m_frame_count; }

        /**
         * @brief Anlık yumuşatılmış saniye başına kare sayısı (FPS).
         */
        [[nodiscard]] float fps() const noexcept { return m_fps; }

        /**
         * @brief Zaman ölçeğini ayarlar (negatif olamaz).
         */
        void set_time_scale(float scale) noexcept
        {
            m_time_scale = (scale >= 0.0f) ? scale : 0.0f;
        }

        /**
         * @brief Sabit fizik simülasyon adım süresini ayarlar (ör. 1.0f / 60.0f veya 1.0f / 120.0f).
         */
        void set_fixed_delta_time(float fixedDt) noexcept
        {
            if (fixedDt > 0.0f)
            {
                m_fixed_delta_time = fixedDt;
            }
        }

    private:
        using clock = std::chrono::steady_clock;

        clock::time_point m_start_time;
        clock::time_point m_last_time;

        float m_delta_time = 0.016f;
        float m_unscaled_delta_time = 0.016f;
        float m_fixed_delta_time = 1.0f / 60.0f; ///< 60 Hz sabit simülasyon adımı
        float m_total_time = 0.0f;
        float m_time_scale = 1.0f;
        uint64_t m_frame_count = 0;

        double m_accumulator = 0.0;
        float m_max_frame_time = 0.25f; ///< Spiral of death (ölüm sarmalı) koruması: maksimum 250 ms
        int m_fixed_steps_taken_this_frame = 0;
        int m_max_sub_steps = 5;         ///< Tek bir render karesinde izin verilen en fazla fizik adımı

        // FPS hesabı için filtre değişkenleri
        float m_fps = 60.0f;
        float m_fps_timer = 0.0f;
        int m_fps_frame_counter = 0;
    };

} // namespace core
