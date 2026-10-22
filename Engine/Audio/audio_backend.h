#pragma once

/**
 * @file audio_backend.h
 * @brief Ses motoru için donanım ve kütüphane bağımsız saf arayüz (IAudioBackend).
 * 
 * Bu soyutlama sayesinde alt seviye ses kütüphanesi (miniaudio, FMOD, OpenAL, SoLoud vb.)
 * motorun geri kalanından tamamen izole edilir ve gerektiğinde kolayca değiştirilebilir.
 */

#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace audio {

    using sound_id = uint64_t;
    constexpr sound_id invalid_sound_id = 0;

    /**
     * @struct audio_clip_info
     * @brief Ses dosyasına ait temel teknik meta veriler.
     */
    struct audio_clip_info {
        float duration_seconds{ 0.0f }; ///< Ses süresi (saniye).
        uint32_t channels{ 0 };         ///< Kanal sayısı (1: Mono, 2: Stereo).
        uint32_t sample_rate{ 0 };      ///< Örnekleme hızı (Hz, örn: 44100).
    };

    /**
     * @class IAudioBackend
     * @brief Ses arka uçları (Backends) için saf sanal arayüz sınıfı.
     */
    class IAudioBackend {
    public:
        virtual ~IAudioBackend() = default;

        [[nodiscard]] virtual const char* get_backend_name() const { return "miniaudio"; }

        /**
         * @brief Ses sürücüsünü ve cihazı ilklendirir.
         * @return Başarılı ise true, aksi halde false.
         */
        virtual bool init() = 0;

        /**
         * @brief Ses sürücüsünü ve kaynakları kapatır.
         */
        virtual void shutdown() = 0;

        /**
         * @brief Kare başına periyodik ses motoru güncellemesi.
         * @param dt Delta time (saniye).
         */
        virtual void update(float dt) = 0;

        /**
         * @brief 3B ses dinleyicisinin (Listener / Kamera) konum ve yönelimini günceller.
         */
        virtual void set_listener_transform(
            const glm::vec3& position,
            const glm::vec3& forward,
            const glm::vec3& up,
            const glm::vec3& velocity = glm::vec3(0.0f)) = 0;

        /**
         * @brief Bir ses dosyasını yükleyip benzersiz bir ses tanıtıcısı (sound_id) oluşturur.
         * @param filepath Ses dosyasının fiziksel yolu.
         * @param is3D Sesin 3B uzaysal mı yoksa 2B (Stereo/UI/Müzik) mi çalınacağı.
         * @return sound_id Başarılı ise geçerli ID, aksi halde invalid_sound_id.
         */
        virtual sound_id load_sound(const std::string& filepath, bool is3D = true) = 0;

        /**
         * @brief Yüklenmiş bir sesi bellekten ve ses motorundan serbest bırakır.
         */
        virtual void unload_sound(sound_id id) = 0;

        /**
         * @brief Sesi baştan veya kaldığı yerden oynatır.
         */
        virtual void play_sound(sound_id id) = 0;

        /**
         * @brief Sesi durdurur ve başa sarar.
         */
        virtual void stop_sound(sound_id id) = 0;

        /**
         * @brief Sesi duraklatır (konumu korur).
         */
        virtual void pause_sound(sound_id id) = 0;

        /**
         * @brief Duraklatılmış sesi devam ettirir.
         */
        virtual void resume_sound(sound_id id) = 0;

        /**
         * @brief Sesin şu anda çalıp çalmadığını sorgular.
         */
        [[nodiscard]] virtual bool is_sound_playing(sound_id id) const = 0;

        /**
         * @brief Sesin çalma pozisyonunu (saniye) döner.
         */
        [[nodiscard]] virtual float get_sound_cursor(sound_id id) const = 0;

        /**
         * @brief Sesin çalma pozisyonunu ayarlar.
         */
        virtual void set_sound_cursor(sound_id id, float seconds) = 0;

        /**
         * @brief Ses şiddetini ayarlar (0.0f = Sessiz, 1.0f = Standart).
         */
        virtual void set_sound_volume(sound_id id, float volume) = 0;

        /**
         * @brief Ses perdesini / hızını ayarlar (1.0f = Normal).
         */
        virtual void set_sound_pitch(sound_id id, float pitch) = 0;

        /**
         * @brief Sesin sürekli döngüde (loop) çalıp çalmayacağını belirler.
         */
        virtual void set_sound_looping(sound_id id, bool loop) = 0;

        /**
         * @brief 3B ses kaynağının dünya uzayındaki konumunu ayarlar.
         */
        virtual void set_sound_position(sound_id id, const glm::vec3& position) = 0;

        /**
         * @brief 3B ses kaynağının hızını (Doppler etkisi için) ayarlar.
         */
        virtual void set_sound_velocity(sound_id id, const glm::vec3& velocity) = 0;

        /**
         * @brief 3B ses zayıflama (attenuation) parametrelerini ayarlar.
         * @param minDistance Zayıflamanın başladığı minimum mesafe.
         * @param maxDistance Sesin tamamen kaybolduğu maksimum mesafe.
         * @param rollOff Mesafe azalım katsayısı.
         */
        virtual void set_sound_attenuation(sound_id id, float minDistance, float maxDistance, float rollOff) = 0;

        /**
         * @brief 3B uzaysal sesin (Spatialization) dinamik olarak etkinlestirilmesini/kapatilmasini saglar.
         */
        virtual void set_sound_spatialization_enabled(sound_id id, bool enabled) = 0;

        /**
         * @brief Tek seferlik (Fire-and-forget) ses oynatır (patlama, UI tıklama, vuruş efekti vb.).
         * @param filepath Ses dosyası yolu.
         * @param volume Ses seviyesi.
         * @param position Opsiyonel 3B konum (nullptr ise 2B çalınır).
         */
        virtual void play_one_shot(const std::string& filepath, float volume = 1.0f, const glm::vec3* position = nullptr) = 0;

        /**
         * @brief Ana ses çıkış seviyesini (Master Volume) ayarlar (0.0f - 1.0f).
         */
        virtual void set_master_volume(float volume) = 0;

        /**
         * @brief Ana ses çıkış seviyesini sorgular.
         */
        [[nodiscard]] virtual float get_master_volume() const = 0;

        /**
         * @brief Çalan tüm ses kaynaklarını durdurur.
         */
        virtual void stop_all() = 0;

        /**
         * @brief Belirtilen ses dosyasının meta verilerini (süre, kanal vb.) okur.
         */
        virtual bool get_sound_info(const std::string& filepath, audio_clip_info& outInfo) = 0;
    };

} // namespace audio

