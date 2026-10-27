#pragma once

/**
 * @file log_types.h
 * @brief Merkezi loglama sistemi için seviye, kategori ve sabit bellekli log girdisi tanımları.
 */

#include <cstdint>
#include <string_view>

namespace qual::log {

/**
 * @enum log_level
 * @brief Log mesajının önem ve ciddiyet derecesi.
 */
enum class log_level : uint8_t {
    trace = 0,
    debug,
    info,
    warn,
    error,
    critical
};

/**
 * @enum log_category
 * @brief Log mesajının kaynaklandığı motor alt sistemi.
 */
enum class log_category : uint8_t {
    core = 0,
    renderer,
    physics,
    audio,
    script,
    asset,
    editor,
    system
};

/**
 * @struct log_entry
 * @brief Sıfır heap tahsisi garantili, sabit boyutlu log kaydı.
 */
struct log_entry {
    uint64_t timestamp_ms = 0;           ///< Epoch başlangıcından itibaren geçen milisaniye.
    log_level level = log_level::info;   ///< Mesajın önem derecesi.
    log_category category = log_category::core; ///< Kaynak alt sistem.
    uint32_t line = 0;                   ///< Kaynak kod satır numarası.
    uint32_t entity_id = 0;              ///< İlgili ECS varlık kimliği (Varsa, yoksa 0).
    uint32_t repeat_count = 1;           ///< Log seli (spam) önleme tekrar sayacı.
    char message[256] = { 0 };           ///< Sabit boyutlu metin tamponu (Sıfır dinamik tahsis).
    char file[48] = { 0 };               ///< Dosya adı tamponu.
};

/**
 * @brief Log seviyesini okunabilir metne dönüştürür.
 */
[[nodiscard]] constexpr const char* to_string(log_level lvl) noexcept {
    switch (lvl) {
        case log_level::trace:    return "TRACE";
        case log_level::debug:    return "DEBUG";
        case log_level::info:     return "INFO";
        case log_level::warn:     return "WARN";
        case log_level::error:    return "ERROR";
        case log_level::critical: return "FATAL";
        default:                  return "UNKNOWN";
    }
}

/**
 * @brief Log kategorisini okunabilir metne dönüştürür.
 */
[[nodiscard]] constexpr const char* to_string(log_category cat) noexcept {
    switch (cat) {
        case log_category::core:     return "Core";
        case log_category::renderer: return "Renderer";
        case log_category::physics:  return "Physics";
        case log_category::audio:    return "Audio";
        case log_category::script:   return "Script";
        case log_category::asset:    return "Asset";
        case log_category::editor:   return "Editor";
        case log_category::system:   return "System";
        default:                     return "General";
    }
}

} // namespace qual::log

