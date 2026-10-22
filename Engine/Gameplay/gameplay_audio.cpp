/**
 * @file gameplay_audio.cpp
 * @brief Gameplay API ses fonksiyonlarÄ±nÄ±n uygulamasÄ±.
 */

#include "gameplay_audio.h"
#include "gameplay_context.h"
#include "../Engine/engine_context.h"
#include "../Scene/scene.h"
#include "../Audio/audio_components.h"

namespace gameplay {

    static EngineContext* get_ctx() {
        return context::get_engine_context();
    }

    static scene* get_scene() {
        return context::get_active_scene();
    }

    // ========================================================================
    // AUDIO SOURCE SARMALAYICI (WRAPPER)
    // ========================================================================

    audio_source::audio_source(entity e)
        : m_entity(e)
    {
    }

    bool audio_source::is_valid() const
    {
        scene* s = get_scene();
        if (!s || m_entity == 0) return false;
        return s->get_registry().has_component<audio_source_component>(m_entity);
    }

    void audio_source::play()
    {
        EngineContext* ctx = get_ctx();
        scene* s = get_scene();
        if (!ctx || !s || !is_valid()) return;

        ctx->audioSystem.play_source(m_entity, s->get_registry());
    }

    void audio_source::stop()
    {
        EngineContext* ctx = get_ctx();
        scene* s = get_scene();
        if (!ctx || !s || !is_valid()) return;

        ctx->audioSystem.stop_source(m_entity, s->get_registry());
    }

    void audio_source::pause()
    {
        EngineContext* ctx = get_ctx();
        scene* s = get_scene();
        if (!ctx || !s || !is_valid()) return;

        ctx->audioSystem.pause_source(m_entity, s->get_registry());
    }

    void audio_source::resume()
    {
        EngineContext* ctx = get_ctx();
        scene* s = get_scene();
        if (!ctx || !s || !is_valid()) return;

        ctx->audioSystem.resume_source(m_entity, s->get_registry());
    }

    bool audio_source::is_playing() const
    {
        EngineContext* ctx = get_ctx();
        scene* s = get_scene();
        if (!ctx || !s || !is_valid()) return false;

        return ctx->audioSystem.is_source_playing(m_entity, s->get_registry());
    }

    void audio_source::set_volume(float volume)
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return;

        auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        if (src) {
            src->volume = volume;
            EngineContext* ctx = get_ctx();
            if (ctx && ctx->audioSystem.get_backend() && src->runtime_sound_id != ::audio::invalid_sound_id) {
                ctx->audioSystem.get_backend()->set_sound_volume(src->runtime_sound_id, volume);
            }
        }
    }

    float audio_source::get_volume() const
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return 1.0f;

        const auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        return src ? src->volume : 1.0f;
    }

    void audio_source::set_pitch(float pitch)
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return;

        auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        if (src) {
            src->pitch = pitch;
            EngineContext* ctx = get_ctx();
            if (ctx && ctx->audioSystem.get_backend() && src->runtime_sound_id != ::audio::invalid_sound_id) {
                ctx->audioSystem.get_backend()->set_sound_pitch(src->runtime_sound_id, pitch);
            }
        }
    }

    float audio_source::get_pitch() const
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return 1.0f;

        const auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        return src ? src->pitch : 1.0f;
    }

    void audio_source::set_loop(bool loop)
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return;

        auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        if (src) {
            src->loop = loop;
            EngineContext* ctx = get_ctx();
            if (ctx && ctx->audioSystem.get_backend() && src->runtime_sound_id != ::audio::invalid_sound_id) {
                ctx->audioSystem.get_backend()->set_sound_looping(src->runtime_sound_id, loop);
            }
        }
    }

    bool audio_source::is_looping() const
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return false;

        const auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        return src ? src->loop : false;
    }

    void audio_source::set_clip(const std::string& clipPath)
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return;

        auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        if (src) {
            src->clip_path = clipPath;
            if (src->runtime_sound_id != ::audio::invalid_sound_id) {
                EngineContext* ctx = get_ctx();
                if (ctx && ctx->audioSystem.get_backend()) {
                    ctx->audioSystem.get_backend()->stop_sound(src->runtime_sound_id);
                    ctx->audioSystem.get_backend()->unload_sound(src->runtime_sound_id);
                }
                src->runtime_sound_id = ::audio::invalid_sound_id;
            }
        }
    }

    std::string audio_source::get_clip() const
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return "";

        const auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        return src ? src->clip_path : "";
    }

    void audio_source::set_3d(bool is3D)
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return;

        auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        if (src) {
            src->is_3d = is3D;
        }
    }

    bool audio_source::is_3d() const
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return true;

        const auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        return src ? src->is_3d : true;
    }

    void audio_source::set_distance_range(float minDistance, float maxDistance)
    {
        scene* s = get_scene();
        if (!s || !is_valid()) return;

        auto* src = s->get_registry().get_component<audio_source_component>(m_entity);
        if (src) {
            src->min_distance = minDistance;
            src->max_distance = maxDistance;
            EngineContext* ctx = get_ctx();
            if (ctx && ctx->audioSystem.get_backend() && src->runtime_sound_id != ::audio::invalid_sound_id) {
                ctx->audioSystem.get_backend()->set_sound_attenuation(src->runtime_sound_id, minDistance, maxDistance, src->roll_off);
            }
        }
    }

    // ========================================================================
    // GLOBAL AUDIO FONKSÄ°YONLARI
    // ========================================================================

    namespace audio {

        void play(const std::string& clipPath, float volume, float pitch)
        {
            EngineContext* ctx = get_ctx();
            if (!ctx) return;

            ctx->audioSystem.play_one_shot(clipPath, volume, nullptr);
        }

        void play_at_position(const std::string& clipPath, const glm::vec3& position, float volume, float pitch)
        {
            EngineContext* ctx = get_ctx();
            if (!ctx) return;

            ctx->audioSystem.play_one_shot(clipPath, volume, &position);
        }

        void set_master_volume(float volume)
        {
            EngineContext* ctx = get_ctx();
            if (!ctx) return;

            ctx->audioSystem.set_master_volume(volume);
        }

        float get_master_volume()
        {
            EngineContext* ctx = get_ctx();
            if (!ctx) return 1.0f;

            return ctx->audioSystem.get_master_volume();
        }

        void stop_all()
        {
            EngineContext* ctx = get_ctx();
            if (!ctx) return;

            ctx->audioSystem.stop_all();
        }

    } // namespace audio

} // namespace gameplay

