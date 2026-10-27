#pragma once

/**
 * @file logger.h
 * @brief Sıfır heap tahsisli, thread-safe, halka tamponlu ve spam korumalı merkezi log yöneticisi.
 */

#include "log_types.h"
#include <format>
#include <string_view>
#include <array>
#include <vector>
#include <mutex>
#include <chrono>
#include <cstring>
#include <iostream>
#include <utility>

namespace qual::log {

/**
 * @class engine_logger
 * @brief Çok iş parçacıklı güvenli (thread-safe) merkezi log motoru.
 */
class engine_logger {
public:
    static constexpr size_t BUFFER_CAPACITY = 2048;

    /**
     * @brief Singleton örneğine erişim sağlar.
     */
    static engine_logger& instance() noexcept {
        static engine_logger s_instance;
        return s_instance;
    }

    /**
     * @brief Yeni bir log mesajı kaydeder.
     *
     * @tparam Args Biçimlendirme parametreleri.
     * @param level Mesajın ciddiyet seviyesi.
     * @param cat Kaynak kategori.
     * @param file Kaynak kod dosya yolu.
     * @param line Satır numarası.
     * @param entity_id İlgili varlık kimliği (varsa, yoksa 0).
     * @param fmt std::format uyumlu biçimlendirme dizesi.
     * @param args Parametreler.
     */
    template <typename... Args>
    void log(log_level level, log_category cat, const char* file, uint32_t line,
             uint32_t entity_id, std::format_string<Args...> fmt, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 1. Doğrudan yerel sabit tampona biçimlendir (Sıfır Dinamik Heap Tahsisi!)
        char temp_msg[256] = { 0 };
        auto result = std::format_to_n(temp_msg, sizeof(temp_msg) - 1, fmt, std::forward<Args>(args)...);
        *result.out = '\0';

        const uint64_t now_ms = get_current_time_ms();

        // 2. Log Seli (Flood / Spam) Engelleme:
        // Eğer aynı satırdan aynı içerikli mesaj ardı ardına geliyorsa (Örn: 144 FPS her kare hata),
        // yeni satır eklemek yerine son girdinin tekrar sayacını artır!
        if (m_count > 0) {
            size_t last_idx = (m_head + BUFFER_CAPACITY - 1) % BUFFER_CAPACITY;
            log_entry& last = m_buffer[last_idx];
            if (last.level == level && last.category == cat && last.line == line &&
                std::strncmp(last.message, temp_msg, sizeof(temp_msg)) == 0)
            {
                last.repeat_count++;
                last.timestamp_ms = now_ms;
                return;
            }
        }

        // 3. Halka Tampona (Circular Ring Buffer) Ekleme
        log_entry& entry = m_buffer[m_head];
        entry.timestamp_ms = now_ms;
        entry.level = level;
        entry.category = cat;
        entry.line = line;
        entry.entity_id = entity_id;
        entry.repeat_count = 1;

        std::memcpy(entry.message, temp_msg, sizeof(entry.message));

        std::string_view fname = extract_filename(file);
        size_t copy_len = (fname.size() < sizeof(entry.file) - 1) ? fname.size() : sizeof(entry.file) - 1;
        std::memcpy(entry.file, fname.data(), copy_len);
        entry.file[copy_len] = '\0';

        m_head = (m_head + 1) % BUFFER_CAPACITY;
        if (m_count < BUFFER_CAPACITY) {
            m_count++;
        }

        // 4. Standart Çıktı Akışına Aynalama (Mirroring)
        // Konsol veya debugger kullanan geliştiriciler için stdout/stderr akışına da yazılır
        std::ostream& out = (level >= log_level::warn) ? std::cerr : std::cout;
        out << "[" << to_string(level) << "] [" << to_string(cat) << "] "
            << entry.message << " (" << entry.file << ":" << entry.line << ")\n";
    }

    /**
     * @brief UI arayüzü için kilit altında güvenli anlık görüntü (snapshot) kopyası alır.
     *
     * @note Data race (veri yarışı) oluşmaması için arayüz doğrudan halka tampon göstericisi değil,
     * bu anlık kopya üzerinden çizim yapar.
     * @param out_snapshot Kronolojik sırayla kopyalanacak log vektörü.
     */
    void get_snapshot(std::vector<log_entry>& out_snapshot) {
        std::lock_guard<std::mutex> lock(m_mutex);
        out_snapshot.clear();
        out_snapshot.reserve(m_count);

        size_t start = (m_count < BUFFER_CAPACITY) ? 0 : m_head;
        for (size_t i = 0; i < m_count; ++i) {
            size_t idx = (start + i) % BUFFER_CAPACITY;
            out_snapshot.push_back(m_buffer[idx]);
        }
    }

    /**
     * @brief Tüm kayıtlı log geçmişini temizler.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_count = 0;
    }

    /**
     * @brief Kayıtlı toplam log adedini döndürür.
     */
    [[nodiscard]] size_t get_log_count() const noexcept {
        return m_count;
    }

private:
    engine_logger() = default;
    ~engine_logger() = default;

    engine_logger(const engine_logger&) = delete;
    engine_logger& operator=(const engine_logger&) = delete;

    static uint64_t get_current_time_ms() noexcept {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
    }

    static std::string_view extract_filename(const char* full_path) noexcept {
        if (!full_path) return "";
        std::string_view v(full_path);
        size_t pos = v.find_last_of("/\\");
        return (pos == std::string_view::npos) ? v : v.substr(pos + 1);
    }

    mutable std::mutex m_mutex;
    std::array<log_entry, BUFFER_CAPACITY> m_buffer{};
    size_t m_head = 0;
    size_t m_count = 0;
};

} // namespace qual::log

// ============================================================================
// ERGONOMİK MERKEZİ LOG MAKROLARI (Tüm Derleyicilerle %100 Uyumlu)
// ============================================================================

#define QLOG_TRACE(cat, ...) \
    qual::log::engine_logger::instance().log(qual::log::log_level::trace, qual::log::log_category::cat, __FILE__, __LINE__, 0, __VA_ARGS__)

#define QLOG_DEBUG(cat, ...) \
    qual::log::engine_logger::instance().log(qual::log::log_level::debug, qual::log::log_category::cat, __FILE__, __LINE__, 0, __VA_ARGS__)

#define QLOG_INFO(cat, ...) \
    qual::log::engine_logger::instance().log(qual::log::log_level::info, qual::log::log_category::cat, __FILE__, __LINE__, 0, __VA_ARGS__)

#define QLOG_WARN(cat, ...) \
    qual::log::engine_logger::instance().log(qual::log::log_level::warn, qual::log::log_category::cat, __FILE__, __LINE__, 0, __VA_ARGS__)

#define QLOG_ERROR(cat, ...) \
    qual::log::engine_logger::instance().log(qual::log::log_level::error, qual::log::log_category::cat, __FILE__, __LINE__, 0, __VA_ARGS__)

#define QLOG_CRITICAL(cat, ...) \
    qual::log::engine_logger::instance().log(qual::log::log_level::critical, qual::log::log_category::cat, __FILE__, __LINE__, 0, __VA_ARGS__)

#define QLOG_SCRIPT_ERROR(entity_id, ...) \
    qual::log::engine_logger::instance().log(qual::log::log_level::error, qual::log::log_category::script, __FILE__, __LINE__, entity_id, __VA_ARGS__)
