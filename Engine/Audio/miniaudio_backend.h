#pragma once

/**
 * @file miniaudio_backend.h
 * @brief Miniaudio kütüphanesini kullanan IAudioBackend somut uygulaması.
 * @note PImpl deseni ile miniaudio.h dahili tutulur, motor geneli derleme süresi korunur.
 */

#include "audio_backend.h"
#include <memory>

namespace audio {

    class miniaudio_backend final : public IAudioBackend {
    public:
        miniaudio_backend();
        ~miniaudio_backend() override;

        bool init() override;
        void shutdown() override;
        void update(float dt) override;

        void set_listener_transform(
            const glm::vec3& position,
            const glm::vec3& forward,
            const glm::vec3& up,
            const glm::vec3& velocity = glm::vec3(0.0f)) override;

        sound_id load_sound(const std::string& filepath, bool is3D = true) override;
        void unload_sound(sound_id id) override;

        void play_sound(sound_id id) override;
        void stop_sound(sound_id id) override;
        void pause_sound(sound_id id) override;
        void resume_sound(sound_id id) override;

        [[nodiscard]] bool is_sound_playing(sound_id id) const override;
        [[nodiscard]] float get_sound_cursor(sound_id id) const override;
        void set_sound_cursor(sound_id id, float seconds) override;

        void set_sound_volume(sound_id id, float volume) override;
        void set_sound_pitch(sound_id id, float pitch) override;
        void set_sound_looping(sound_id id, bool loop) override;

        void set_sound_position(sound_id id, const glm::vec3& position) override;
        void set_sound_velocity(sound_id id, const glm::vec3& velocity) override;
        void set_sound_attenuation(sound_id id, float minDistance, float maxDistance, float rollOff) override;
        void set_sound_spatialization_enabled(sound_id id, bool enabled) override;

        void play_one_shot(const std::string& filepath, float volume = 1.0f, const glm::vec3* position = nullptr) override;

        void set_master_volume(float volume) override;
        [[nodiscard]] float get_master_volume() const override;

        void stop_all() override;

        bool get_sound_info(const std::string& filepath, audio_clip_info& outInfo) override;

    private:
        struct impl;
        std::unique_ptr<impl> m_impl;
    };

} // namespace audio

