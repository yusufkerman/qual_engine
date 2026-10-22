/**
 * @file miniaudio_backend.cpp
 * @brief Miniaudio tabanlı ses arka ucu uygulaması.
 */

#define MINIAUDIO_IMPLEMENTATION
#include "../Externals/miniaudio.h"

#include "miniaudio_backend.h"
#include "../Resources/engine_resources.h"
#include "../Project/project.h"
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <filesystem>

namespace {
    std::string resolve_audio_path(const std::string& filepath)
    {
        if (filepath.empty()) return "";
        try {
            if (std::filesystem::exists(filepath)) return filepath;

            std::string res = engine_resources::resolve_path(filepath);
            if (std::filesystem::exists(res)) return res;

            if (project::project_manager::instance().has_active_project()) {
                const auto& root = project::project_manager::instance().get_project_root();
                auto p1 = std::filesystem::path(root) / filepath;
                if (std::filesystem::exists(p1)) return p1.string();

                auto p2 = std::filesystem::path(root) / "Assets" / filepath;
                if (std::filesystem::exists(p2)) return p2.string();

                if (filepath.rfind("Assets/", 0) == 0 || filepath.rfind("Assets\\", 0) == 0) {
                    auto p3 = std::filesystem::path(root) / filepath.substr(7);
                    if (std::filesystem::exists(p3)) return p3.string();
                }
            }
        } catch (...) {}
        return filepath;
    }
}

namespace audio {

    struct sound_instance {
        std::unique_ptr<ma_sound> sound;
        std::string filepath;
        bool is3D{ true };
    };

    struct miniaudio_backend::impl {
        ma_engine engine{};
        bool is_initialized{ false };
        float master_volume{ 1.0f };

        mutable std::mutex sound_mutex;
        sound_id next_sound_id{ 1 };
        std::unordered_map<sound_id, sound_instance> active_sounds;
    };

    miniaudio_backend::miniaudio_backend()
        : m_impl(std::make_unique<impl>())
    {
    }

    miniaudio_backend::~miniaudio_backend()
    {
        shutdown();
    }

    bool miniaudio_backend::init()
    {
        if (m_impl->is_initialized) {
            return true;
        }

        ma_engine_config config = ma_engine_config_init();
        // Standart hoparlör / kulaklık çıkışı, varsayılan örnekleme hızı
        ma_result result = ma_engine_init(&config, &m_impl->engine);
        if (result != MA_SUCCESS) {
            std::cerr << "[Audio] Hata: miniaudio motoru ilklendirilemedi. Kod: " << result << "\n";
            return false;
        }

        m_impl->is_initialized = true;
        std::cout << "[Audio] Miniaudio ses motoru basariyla baslatildi.\n";
        return true;
    }

    void miniaudio_backend::shutdown()
    {
        if (!m_impl->is_initialized) {
            return;
        }

        stop_all();

        {
            std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
            for (auto& [id, instance] : m_impl->active_sounds) {
                if (instance.sound) {
                    ma_sound_uninit(instance.sound.get());
                }
            }
            m_impl->active_sounds.clear();
        }

        ma_engine_uninit(&m_impl->engine);
        m_impl->is_initialized = false;
        std::cout << "[Audio] Miniaudio ses motoru kapatildi.\n";
    }

    void miniaudio_backend::update(float /*dt*/)
    {
        // miniaudio asenkron arka plan mikser iş parçacığında çalışır.
    }

    void miniaudio_backend::set_listener_transform(
        const glm::vec3& position,
        const glm::vec3& forward,
        const glm::vec3& up,
        const glm::vec3& velocity)
    {
        if (!m_impl->is_initialized) return;

        ma_engine_listener_set_position(&m_impl->engine, 0, position.x, position.y, position.z);
        ma_engine_listener_set_direction(&m_impl->engine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(&m_impl->engine, 0, up.x, up.y, up.z);
        ma_engine_listener_set_velocity(&m_impl->engine, 0, velocity.x, velocity.y, velocity.z);
    }

    sound_id miniaudio_backend::load_sound(const std::string& filepath, bool is3D)
    {
        if (!m_impl->is_initialized) {
            if (!init()) return invalid_sound_id;
        }

        std::string resolved = resolve_audio_path(filepath);
        if (!std::filesystem::exists(resolved)) {
            std::cerr << "[Audio] Error: Sound file not found: " << filepath << " (Resolved: " << resolved << ")\n";
            return invalid_sound_id;
        }

        auto soundPtr = std::make_unique<ma_sound>();
        ma_uint32 flags = is3D ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;

        ma_result result = ma_sound_init_from_file(
            &m_impl->engine,
            resolved.c_str(),
            flags,
            nullptr,
            nullptr,
            soundPtr.get()
        );

        if (result != MA_SUCCESS) {
            std::cerr << "[Audio] Error: Failed to load sound: " << resolved << " (Hata Kodu: " << result << ")\n";
            return invalid_sound_id;
        }

        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        sound_id id = m_impl->next_sound_id++;

        sound_instance instance;
        instance.sound = std::move(soundPtr);
        instance.filepath = resolved;
        instance.is3D = is3D;

        if (is3D) {
            ma_sound_set_attenuation_model(instance.sound.get(), ma_attenuation_model_linear);
        }

        m_impl->active_sounds[id] = std::move(instance);
        std::cout << "[Audio] Sound loaded successfully: " << resolved << " (id: " << id << ", 3D: " << (is3D ? "true" : "false") << ")\n";
        return id;
    }

    void miniaudio_backend::unload_sound(sound_id id)
    {
        if (id == invalid_sound_id) return;

        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end()) {
            if (it->second.sound) {
                ma_sound_stop(it->second.sound.get());
                ma_sound_uninit(it->second.sound.get());
            }
            m_impl->active_sounds.erase(it);
        }
    }

    void miniaudio_backend::play_sound(sound_id id)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            if (ma_sound_at_end(it->second.sound.get())) {
                ma_sound_seek_to_pcm_frame(it->second.sound.get(), 0);
            }
            ma_result res = ma_sound_start(it->second.sound.get());
            if (res != MA_SUCCESS) {
                std::cerr << "[Audio] Warning: ma_sound_start failed with code " << res << " for sound id " << id << "\n";
            }
        }
    }

    void miniaudio_backend::stop_sound(sound_id id)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_stop(it->second.sound.get());
            ma_sound_seek_to_pcm_frame(it->second.sound.get(), 0);
        }
    }

    void miniaudio_backend::pause_sound(sound_id id)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_stop(it->second.sound.get());
        }
    }

    void miniaudio_backend::resume_sound(sound_id id)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_start(it->second.sound.get());
        }
    }

    bool miniaudio_backend::is_sound_playing(sound_id id) const
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            return ma_sound_is_playing(it->second.sound.get()) == MA_TRUE;
        }
        return false;
    }

    float miniaudio_backend::get_sound_cursor(sound_id id) const
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            float cursor = 0.0f;
            ma_sound_get_cursor_in_seconds(it->second.sound.get(), &cursor);
            return cursor;
        }
        return 0.0f;
    }

    void miniaudio_backend::set_sound_cursor(sound_id id, float seconds)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_seek_to_pcm_frame(
                it->second.sound.get(),
                static_cast<ma_uint64>(seconds * ma_engine_get_sample_rate(&m_impl->engine))
            );
        }
    }

    void miniaudio_backend::set_sound_volume(sound_id id, float volume)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_set_volume(it->second.sound.get(), volume);
        }
    }

    void miniaudio_backend::set_sound_pitch(sound_id id, float pitch)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_set_pitch(it->second.sound.get(), pitch);
        }
    }

    void miniaudio_backend::set_sound_looping(sound_id id, bool loop)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_set_looping(it->second.sound.get(), loop ? MA_TRUE : MA_FALSE);
        }
    }

    void miniaudio_backend::set_sound_position(sound_id id, const glm::vec3& position)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_set_position(it->second.sound.get(), position.x, position.y, position.z);
        }
    }

    void miniaudio_backend::set_sound_velocity(sound_id id, const glm::vec3& velocity)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            ma_sound_set_velocity(it->second.sound.get(), velocity.x, velocity.y, velocity.z);
        }
    }

    void miniaudio_backend::set_sound_attenuation(sound_id id, float minDistance, float maxDistance, float rollOff)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            float safeMax = maxDistance;
            if (safeMax <= minDistance) safeMax = minDistance + 0.1f;
            ma_sound_set_attenuation_model(it->second.sound.get(), ma_attenuation_model_linear);
            ma_sound_set_min_distance(it->second.sound.get(), minDistance);
            ma_sound_set_max_distance(it->second.sound.get(), safeMax);
            ma_sound_set_rolloff(it->second.sound.get(), rollOff);
        }
    }

    void miniaudio_backend::set_sound_spatialization_enabled(sound_id id, bool enabled)
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        auto it = m_impl->active_sounds.find(id);
        if (it != m_impl->active_sounds.end() && it->second.sound) {
            if (it->second.is3D != enabled) {
                ma_sound_set_spatialization_enabled(it->second.sound.get(), enabled ? MA_TRUE : MA_FALSE);
                if (enabled) {
                    ma_sound_set_attenuation_model(it->second.sound.get(), ma_attenuation_model_linear);
                }
                it->second.is3D = enabled;
            }
        }
    }

    void miniaudio_backend::play_one_shot(const std::string& filepath, float volume, const glm::vec3* position)
    {
        if (!m_impl->is_initialized) {
            if (!init()) return;
        }

        std::string resolved = resolve_audio_path(filepath);
        if (!std::filesystem::exists(resolved)) return;

        if (position == nullptr) {
            // 2D One-shot
            ma_engine_play_sound(&m_impl->engine, resolved.c_str(), nullptr);
        }
        else {
            // 3D One-shot
            sound_id sid = load_sound(resolved, true);
            if (sid != invalid_sound_id) {
                set_sound_volume(sid, volume);
                set_sound_position(sid, *position);
                set_sound_looping(sid, false);
                play_sound(sid);
            }
        }
    }

    void miniaudio_backend::set_master_volume(float volume)
    {
        m_impl->master_volume = volume;
        if (m_impl->is_initialized) {
            ma_engine_set_volume(&m_impl->engine, volume);
        }
    }

    float miniaudio_backend::get_master_volume() const
    {
        return m_impl->master_volume;
    }

    void miniaudio_backend::stop_all()
    {
        std::lock_guard<std::mutex> lock(m_impl->sound_mutex);
        for (auto& [id, instance] : m_impl->active_sounds) {
            if (instance.sound) {
                ma_sound_stop(instance.sound.get());
            }
        }
    }

    bool miniaudio_backend::get_sound_info(const std::string& filepath, audio_clip_info& outInfo)
    {
        if (!std::filesystem::exists(filepath)) return false;

        ma_decoder decoder;
        ma_result result = ma_decoder_init_file(filepath.c_str(), nullptr, &decoder);
        if (result != MA_SUCCESS) {
            return false;
        }

        outInfo.channels = decoder.outputChannels;
        outInfo.sample_rate = decoder.outputSampleRate;

        ma_uint64 totalFrames = 0;
        if (ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames) == MA_SUCCESS && decoder.outputSampleRate > 0) {
            outInfo.duration_seconds = static_cast<float>(totalFrames) / static_cast<float>(decoder.outputSampleRate);
        }
        else {
            outInfo.duration_seconds = 0.0f;
        }

        ma_decoder_uninit(&decoder);
        return true;
    }

} // namespace audio

