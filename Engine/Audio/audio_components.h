#pragma once

/**
 * @file audio_components.h
 * @brief Ses sistemi için ECS bileşen tanımları (audio_source_component ve audio_listener_component).
 */

#include "audio_backend.h"
#include "../Engine/Assets/aria.h"
#include <string>

/**
 * @struct audio_source_component
 * @brief Bir varlığa 2B veya 3B uzaysal ses yayma yeteneği kazandıran ECS bileşeni.
 */
struct audio_source_component {
    aria::asset_guid clip_guid;          ///< ARIA ses klibi GUID değeri.
    std::string clip_path;               ///< Ses dosyasının fiziksel/göreceli dosya yolu (.wav, .mp3, .ogg, .flac).

    float volume{ 1.0f };                ///< Ses şiddeti çarpanı (0.0f - 2.0f).
    float pitch{ 1.0f };                 ///< Çalma hızı / frekans perdesi (0.1f - 3.0f).
    bool loop{ false };                  ///< Ses bittiğinde başa dönüp döngüsel çalsın mı?
    bool play_on_awake{ true };          ///< Sahne başladığında otomatik çalsın mı?
    bool is_3d{ true };                  ///< 3B uzaysal mesafe ve yönlendirme hesaplansın mı?

    float min_distance{ 1.0f };          ///< Sesin zayıflamaya başladığı minimum mesafe (metre).
    float max_distance{ 50.0f };         ///< Sesin artık duyulmadığı maksimum mesafe (metre).
    float roll_off{ 1.0f };              ///< Mesafe azalım eğrisi katsayısı.

    // --- Çalışma Zamanı (Runtime) Durum Değişkenleri (Serileştirilmez) ---
    audio::sound_id runtime_sound_id{ audio::invalid_sound_id }; ///< Arka uç ses tanıtıcısı.
    bool is_playing{ false };            ///< Ses şu anda çalıyor mu?
    bool is_paused{ false };             ///< Ses şu anda duraklatılmış mı?
    bool has_played_awake{ false };      ///< Play on awake bir kez tetiklendi mi?
};

/**
 * @struct audio_listener_component
 * @brief 3B dünyadaki kulak / mikrofon konumunu temsil eden dinleyici bileşeni (Genellikle ana kameraya eklenir).
 */
struct audio_listener_component {
    bool is_active{ true };              ///< Bu dinleyici aktif olarak sahneyi dinliyor mu?
};

