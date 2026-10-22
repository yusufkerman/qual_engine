#pragma once

/**
 * @file gameplay_audio.h
 * @brief Oyun betikleri (Gameplay API) için yüksek seviyeli ses çalma ve kontrol arayüzü.
 */

#include <string>
#include <glm/glm.hpp>
#include "../ECS/entity.h"

namespace gameplay {

    /**
     * @class audio_source
     * @brief Bir varlığa ait audio_source_component bileşenini yöneten güvenli gameplay sarmalayıcısı.
     */
    class audio_source {
    public:
        audio_source() = default;
        explicit audio_source(entity e);

        [[nodiscard]] bool is_valid() const;

        void play();
        void stop();
        void pause();
        void resume();

        [[nodiscard]] bool is_playing() const;

        void set_volume(float volume);
        [[nodiscard]] float get_volume() const;

        void set_pitch(float pitch);
        [[nodiscard]] float get_pitch() const;

        void set_loop(bool loop);
        [[nodiscard]] bool is_looping() const;

        void set_clip(const std::string& clipPath);
        [[nodiscard]] std::string get_clip() const;

        void set_3d(bool is3D);
        [[nodiscard]] bool is_3d() const;

        void set_distance_range(float minDistance, float maxDistance);

    private:
        entity m_entity{ 0 };
    };

    /**
     * @namespace audio
     * @brief Global ve tek seferlik (One-Shot) ses çalma fonksiyonları.
     */
    namespace audio {

        /**
         * @brief 2B (Stereo / UI / Müzik) tek seferlik ses çalar.
         * @param clipPath Ses dosyasının yolu (.wav, .mp3, .ogg, .flac).
         * @param volume Ses şiddeti (0.0f - 1.0f).
         * @param pitch Çalma perdesi/hızı.
         */
        void play(const std::string& clipPath, float volume = 1.0f, float pitch = 1.0f);

        /**
         * @brief 3B dünyada belirtilen konumda tek seferlik uzaysal ses çalar.
         * @param clipPath Ses dosyasının yolu.
         * @param position 3B dünya koordinatı.
         * @param volume Ses şiddeti.
         * @param pitch Çalma perdesi.
         */
        void play_at_position(const std::string& clipPath, const glm::vec3& position, float volume = 1.0f, float pitch = 1.0f);

        /**
         * @brief Ana ses seviyesini (Master Volume) ayarlar (0.0f - 1.0f).
         */
        void set_master_volume(float volume);

        /**
         * @brief Ana ses seviyesini sorgular.
         */
        [[nodiscard]] float get_master_volume();

        /**
         * @brief Çalmakta olan tüm sesleri durdurur.
         */
        void stop_all();

    } // namespace audio

} // namespace gameplay

