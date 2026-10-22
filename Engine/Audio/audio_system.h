#pragma once

/**
 * @file audio_system.h
 * @brief Donanım soyutlama katmanı (IAudioBackend) ile ECS bileşenlerini birbirine bağlayan Ses Sistemi.
 */

#include "audio_backend.h"
#include "audio_components.h"
#include "../ECS/registry.h"
#include <memory>

class aria_system;

namespace audio {

    class audio_system {
    public:
        audio_system();
        ~audio_system();

        /**
         * @brief Ses alt yapısını (Backend) ilklendirir.
         * @param customBackend İsteğe bağlı özel backend (nullptr ise miniaudio_backend oluşturulur).
         */
        bool init(std::unique_ptr<IAudioBackend> customBackend = nullptr);

        /**
         * @brief Ses alt yapısını kapatır ve tüm kaynakları boşaltır.
         */
        void shutdown();

        /**
         * @brief Kare başına periyodik ses ve dinleyici güncellemesi.
         * @param dt Delta time (saniye).
         * @param reg ECS varlık kayıt defteri.
         * @param isPlaying Oyun modu aktif mi (Play Mode)?
         */
        void update(float dt, registry& reg, bool isPlaying);

        /**
         * @brief Belirtilen varlığın audio_source_component bileşenini çalar.
         */
        void play_source(entity e, registry& reg, bool force2DPreview = false);

        /**
         * @brief Belirtilen varlığın sesini durdurur ve başa sarar.
         */
        void stop_source(entity e, registry& reg);

        /**
         * @brief Belirtilen varlığın sesini duraklatır.
         */
        void pause_source(entity e, registry& reg);

        /**
         * @brief Duraklatılmış sesi devam ettirir.
         */
        void resume_source(entity e, registry& reg);

        /**
         * @brief Varlığın sesinin çalıp çalmadığını sorgular.
         */
        [[nodiscard]] bool is_source_playing(entity e, const registry& reg) const;

        /**
         * @brief Tek seferlik (One-Shot) ses çalar.
         */
        void play_one_shot(const std::string& filepath, float volume = 1.0f, const glm::vec3* position = nullptr);

        /**
         * @brief Ana ses şiddetini ayarlar.
         */
        void set_master_volume(float volume);

        /**
         * @brief Ana ses şiddetini sorgular.
         */
        [[nodiscard]] float get_master_volume() const;

        /**
         * @brief Tüm sesleri durdurur.
         */
        void stop_all();

        /**
         * @brief Oyun modu durdurulduğunda (Stop Mode) canlı sesleri susturur ve sıfırlar.
         */
        void on_stop_mode(registry& reg);

        /**
         * @brief Alt seviye ses arka ucuna doğrudan erişim sağlar.
         */
        [[nodiscard]] IAudioBackend* get_backend() const { return m_backend.get(); }

    private:
        std::unique_ptr<IAudioBackend> m_backend;
        bool m_is_initialized{ false };
    };

} // namespace audio

