/**
 * @file audio_system.cpp
 * @brief Ses sistemi mantÄ±ksal uygulamasÄ±. ECS dinleyici ve kaynak gÃ¼ncellemelerini yÃ¶netir.
 */

#include "audio_system.h"
#include "miniaudio_backend.h"
#include "../ECS/components.h"
#include "../ECS/transform_utils.h"
#include <iostream>

namespace audio {

    audio_system::audio_system() = default;

    audio_system::~audio_system()
    {
        shutdown();
    }

    bool audio_system::init(std::unique_ptr<IAudioBackend> customBackend)
    {
        if (m_is_initialized) {
            return true;
        }

        if (customBackend) {
            m_backend = std::move(customBackend);
        }
        else {
            m_backend = std::make_unique<miniaudio_backend>();
        }

        if (!m_backend->init()) {
            std::cerr << "[AudioSystem] Hata: Ses arka ucu (backend) baslatilamadi.\n";
            m_backend.reset();
            return false;
        }

        m_is_initialized = true;
        std::cout << "[AudioSystem] Ses alt sistemi (" << m_backend->get_backend_name() << ") basariyla baslatildi.\n";
        return true;
    }

    void audio_system::shutdown()
    {
        if (!m_is_initialized) return;

        if (m_backend) {
            m_backend->shutdown();
            m_backend.reset();
        }

        m_is_initialized = false;
    }

    void audio_system::update(float dt, registry& reg, bool isPlaying)
    {
        if (!m_is_initialized || !m_backend) return;

        m_backend->update(dt);

        // 1. Dinleyici (Audio Listener) Konum ve Yönelimini Güncelle (Yalnızca PLAY modundayken sahne varlıklarından oku)
        if (isPlaying) {
            bool listenerFound = false;
            for (const auto& [e, listener] : reg.get_all<audio_listener_component>()) {
                if (!listener.is_active) continue;

                const auto* t = reg.get_component<transform_component>(e);
                if (t) {
                    const glm::mat4 wm = ecs_transform::get_world_matrix(e, reg);
                    const glm::vec3 pos = glm::vec3(wm[3]);

                    glm::vec3 forward = -glm::vec3(wm[2]);
                    if (glm::length(forward) > 0.0001f) forward = glm::normalize(forward);
                    else forward = glm::vec3(0.0f, 0.0f, -1.0f);

                    glm::vec3 up = glm::vec3(wm[1]);
                    if (glm::length(up) > 0.0001f) up = glm::normalize(up);
                    else up = glm::vec3(0.0f, 1.0f, 0.0f);

                    m_backend->set_listener_transform(pos, forward, up);
                    listenerFound = true;
                    break; // İlk aktif dinleyici kullanılır
                }
            }

            if (!listenerFound) {
                // Dinleyici bileşeni yoksa sahnedeki ana kamerayı dinleyici olarak kullan
                for (const auto& [e, cam] : reg.get_all<camera_component>()) {
                    const auto* t = reg.get_component<transform_component>(e);
                    if (t) {
                        const glm::mat4 wm = ecs_transform::get_world_matrix(e, reg);
                        const glm::vec3 pos = glm::vec3(wm[3]);

                        glm::vec3 forward = -glm::vec3(wm[2]);
                        if (glm::length(forward) > 0.0001f) forward = glm::normalize(forward);
                        else forward = glm::vec3(0.0f, 0.0f, -1.0f);

                        glm::vec3 up = glm::vec3(wm[1]);
                        if (glm::length(up) > 0.0001f) up = glm::normalize(up);
                        else up = glm::vec3(0.0f, 1.0f, 0.0f);

                        m_backend->set_listener_transform(pos, forward, up);
                        listenerFound = true;
                        break;
                    }
                }
            }

            if (!listenerFound) {
                // Dinleyici veya kamera yoksa varsayılan olarak (0, 0, 0) kullanılır
                m_backend->set_listener_transform(
                    glm::vec3(0.0f),
                    glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
            }
        }

        // 2. Ses Kaynaklarını (Audio Sources) Güncelle
        for (auto& [e, src] : reg.get_all<audio_source_component>()) {
            const auto* t = reg.get_component<transform_component>(e);

            // Play On Awake Tetiklemesi (Yalnızca oyun modu aktifken ve henüz tetiklenmediyse)
            if (isPlaying && src.play_on_awake && !src.has_played_awake) {
                src.has_played_awake = true;
                play_source(e, reg);
            }

            // Canlı sesin 3B konumunu, ses seviyesini ve perdesini senkronize et
            if (src.runtime_sound_id != invalid_sound_id) {
                m_backend->set_sound_spatialization_enabled(src.runtime_sound_id, src.is_3d);

                if (t && src.is_3d) {
                    const glm::mat4 wm = ecs_transform::get_world_matrix(e, reg);
                    m_backend->set_sound_position(src.runtime_sound_id, glm::vec3(wm[3]));
                }

                m_backend->set_sound_volume(src.runtime_sound_id, src.volume);
                m_backend->set_sound_pitch(src.runtime_sound_id, src.pitch);
                m_backend->set_sound_looping(src.runtime_sound_id, src.loop);
                m_backend->set_sound_attenuation(src.runtime_sound_id, src.min_distance, src.max_distance, src.roll_off);

                src.is_playing = m_backend->is_sound_playing(src.runtime_sound_id);
            }
            else {
                src.is_playing = false;
            }
        }
    }

    void audio_system::play_source(entity e, registry& reg, bool force2DPreview)
    {
        if (!m_is_initialized || !m_backend) return;

        auto* src = reg.get_component<audio_source_component>(e);
        if (!src) return;

        if (src->clip_path.empty()) {
            return;
        }

        const bool effective3D = force2DPreview ? false : src->is_3d;

        // Eğer ses henüz yüklenmemişse yükle
        if (src->runtime_sound_id == invalid_sound_id) {
            src->runtime_sound_id = m_backend->load_sound(src->clip_path, effective3D);
        }
        else {
            m_backend->set_sound_spatialization_enabled(src->runtime_sound_id, effective3D);
        }

        if (src->runtime_sound_id != invalid_sound_id) {
            const auto* t = reg.get_component<transform_component>(e);
            if (t && effective3D) {
                const glm::mat4 wm = ecs_transform::get_world_matrix(e, reg);
                m_backend->set_sound_position(src->runtime_sound_id, glm::vec3(wm[3]));
            }

            m_backend->set_sound_volume(src->runtime_sound_id, src->volume);
            m_backend->set_sound_pitch(src->runtime_sound_id, src->pitch);
            m_backend->set_sound_looping(src->runtime_sound_id, src->loop);
            m_backend->set_sound_attenuation(src->runtime_sound_id, src->min_distance, src->max_distance, src->roll_off);

            m_backend->play_sound(src->runtime_sound_id);
            src->is_playing = true;
            src->is_paused = false;
        }
    }

    void audio_system::stop_source(entity e, registry& reg)
    {
        if (!m_is_initialized || !m_backend) return;

        auto* src = reg.get_component<audio_source_component>(e);
        if (!src || src->runtime_sound_id == invalid_sound_id) return;

        m_backend->stop_sound(src->runtime_sound_id);
        src->is_playing = false;
        src->is_paused = false;
    }

    void audio_system::pause_source(entity e, registry& reg)
    {
        if (!m_is_initialized || !m_backend) return;

        auto* src = reg.get_component<audio_source_component>(e);
        if (!src || src->runtime_sound_id == invalid_sound_id) return;

        m_backend->pause_sound(src->runtime_sound_id);
        src->is_playing = false;
        src->is_paused = true;
    }

    void audio_system::resume_source(entity e, registry& reg)
    {
        if (!m_is_initialized || !m_backend) return;

        auto* src = reg.get_component<audio_source_component>(e);
        if (!src || src->runtime_sound_id == invalid_sound_id) return;

        m_backend->resume_sound(src->runtime_sound_id);
        src->is_playing = true;
        src->is_paused = false;
    }

    bool audio_system::is_source_playing(entity e, const registry& reg) const
    {
        if (!m_is_initialized || !m_backend) return false;
        const auto* src = reg.get_component<audio_source_component>(e);
        if (!src || src->runtime_sound_id == invalid_sound_id) return false;
        return m_backend->is_sound_playing(src->runtime_sound_id);
    }

    void audio_system::play_one_shot(const std::string& filepath, float volume, const glm::vec3* position)
    {
        if (!m_is_initialized || !m_backend) return;
        m_backend->play_one_shot(filepath, volume, position);
    }

    void audio_system::set_master_volume(float volume)
    {
        if (!m_is_initialized || !m_backend) return;
        m_backend->set_master_volume(volume);
    }

    float audio_system::get_master_volume() const
    {
        if (!m_is_initialized || !m_backend) return 1.0f;
        return m_backend->get_master_volume();
    }

    void audio_system::stop_all()
    {
        if (!m_is_initialized || !m_backend) return;
        m_backend->stop_all();
    }

    void audio_system::on_stop_mode(registry& reg)
    {
        if (!m_is_initialized || !m_backend) return;

        stop_all();

        for (auto& [e, src] : reg.get_all<audio_source_component>()) {
            if (src.runtime_sound_id != invalid_sound_id) {
                m_backend->unload_sound(src.runtime_sound_id);
                src.runtime_sound_id = invalid_sound_id;
            }
            src.is_playing = false;
            src.is_paused = false;
            src.has_played_awake = false;
        }
    }

} // namespace audio
