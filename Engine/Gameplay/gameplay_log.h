#pragma once

/**
 * @file gameplay_log.h
 * @brief Oyun betikleri (Gameplay scripts) için doğrudan Vesta Editör konsoluna
 * ve standart çıktıya yazan ergonomik loglama API'si.
 */

#include "../Core/Logger/logger.h"
#include <string>
#include <format>
#include <utility>

namespace gameplay::log
{
    /**
     * @brief Vesta Konsolu'nda 'Script' kategorisinde bilgilendirme (Info) logu oluşturur.
     * @param fmt std::format uyumlu metin şablonu.
     * @param args Parametreler.
     */
    template <typename... Args>
    inline void info(std::format_string<Args...> fmt, Args&&... args)
    {
        qual::log::engine_logger::instance().log(
            qual::log::log_level::info,
            qual::log::log_category::script,
            __FILE__,
            __LINE__,
            0,
            fmt,
            std::forward<Args>(args)...
        );
    }

    /**
     * @brief Vesta Konsolu'nda 'Script' kategorisinde uyarı (Warn) logu oluşturur.
     * @param fmt std::format uyumlu metin şablonu.
     * @param args Parametreler.
     */
    template <typename... Args>
    inline void warn(std::format_string<Args...> fmt, Args&&... args)
    {
        qual::log::engine_logger::instance().log(
            qual::log::log_level::warn,
            qual::log::log_category::script,
            __FILE__,
            __LINE__,
            0,
            fmt,
            std::forward<Args>(args)...
        );
    }

    /**
     * @brief Vesta Konsolu'nda 'Script' kategorisinde hata (Error) logu oluşturur.
     * @param fmt std::format uyumlu metin şablonu.
     * @param args Parametreler.
     */
    template <typename... Args>
    inline void error(std::format_string<Args...> fmt, Args&&... args)
    {
        qual::log::engine_logger::instance().log(
            qual::log::log_level::error,
            qual::log::log_category::script,
            __FILE__,
            __LINE__,
            0,
            fmt,
            std::forward<Args>(args)...
        );
    }

} // namespace gameplay::log

