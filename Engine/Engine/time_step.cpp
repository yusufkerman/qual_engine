/**
 * @file time_step.cpp
 * @brief Yüksek çözünürlüklü zamanlayıcı ve accumulator mantığının uygulaması.
 */

#include "time_step.h"
#include <algorithm>

namespace core {

    time_step::time_step()
        : m_start_time(clock::now())
        , m_last_time(m_start_time)
    {
    }

    void time_step::tick()
    {
        const auto now = clock::now();
        const std::chrono::duration<double> elapsed = now - m_last_time;
        m_last_time = now;

        double frameTime = elapsed.count();

        // 1. Spiral of Death (Ölüm Sarmalı) Koruması:
        // Pencere taşıma, modal diyaloglar veya breakpoint duraklamalarında biriken sürenin
        // simülasyonu kilitlemesini engellemek için tavan değerle sınırla (ör. max 250 ms)
        if (frameTime > static_cast<double>(m_max_frame_time))
        {
            frameTime = static_cast<double>(m_max_frame_time);
        }

        m_unscaled_delta_time = static_cast<float>(frameTime);
        m_delta_time = m_unscaled_delta_time * m_time_scale;
        m_total_time = std::chrono::duration<float>(now - m_start_time).count();
        m_frame_count++;

        // 2. Glenn Fiedler Accumulator Eklemesi:
        // Fizik motoru için ölçeklendirilmiş süreyi biriktir
        m_accumulator += static_cast<double>(m_delta_time);
        m_fixed_steps_taken_this_frame = 0;

        // 3. FPS Ölçümü (Her 0.5 saniyede bir ortalama yenilenir)
        m_fps_frame_counter++;
        m_fps_timer += m_unscaled_delta_time;
        if (m_fps_timer >= 0.5f)
        {
            m_fps = static_cast<float>(m_fps_frame_counter) / m_fps_timer;
            m_fps_frame_counter = 0;
            m_fps_timer = 0.0f;
        }
    }

    bool time_step::consume_fixed_step()
    {
        if (m_accumulator >= static_cast<double>(m_fixed_delta_time) &&
            m_fixed_steps_taken_this_frame < m_max_sub_steps)
        {
            m_accumulator -= static_cast<double>(m_fixed_delta_time);
            m_fixed_steps_taken_this_frame++;
            return true;
        }

        return false;
    }

    void time_step::reset_accumulator()
    {
        m_accumulator = 0.0;
        m_fixed_steps_taken_this_frame = 0;
    }

} // namespace core
