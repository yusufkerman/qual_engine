#pragma once

/**
 * @file crash_handler.h
 * @brief QualEngine post-mortem çökme yakalama ve Minidump (.dmp) üretim mimarisi.
 * @details İşlenmeyen SEH istisnaları, CRT abort, std::terminate, geçersiz parametre
 * ve saf sanal fonksiyon çağrılarını yakalayarak 'Crashes/' klasörüne yığın ve bellek dökümü üretir.
 */

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <cstdint>
#include <string_view>

namespace qual::core {

/**
 * @class crash_handler
 * @brief Motor seviyesindeki ölümcül çökmeleri yakalayıp minidump üreten statik yönetici.
 */
class crash_handler {
public:
    /**
     * @brief Tüm SEH ve CRT çökme kancalarını (hooks) bağlar ve 'Crashes/' dizinini hazırlar.
     */
    static void init();

    /**
     * @brief Minidump üretimini manuel veya kancadan tetikler.
     * @param ep Win32 EXCEPTION_POINTERS göstericisi (isteğe bağlı).
     * @param reason Çökme nedeni açıklaması.
     * @return true Minidump başarıyla diske yazıldıysa.
     */
    static bool write_minidump(void* ep = nullptr, const char* reason = "Kritik Sistem Cokmesi");

    /**
     * @brief Debugger bağlıyken kancaların hatayı yutmasını önler mi?
     * @note Varsayılan: true (Debugger bağlıyken Visual Studio doğrudan hata satırında durur).
     */
    inline static bool s_pass_through_debugger = true;

    /**
     * @brief Çökme döküm dosyalarının kaydedileceği hedef klasör.
     */
    inline static const char* s_crash_dump_dir = "Crashes";

private:
#ifdef _WIN32
    static LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* ep);
    static void invalid_parameter_handler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved);
    static void purecall_handler();
    static void terminate_handler();
    static void abort_signal_handler(int signal);

    static void format_timestamp(char* out_buf, size_t buf_size);
#endif
};

} // namespace qual::core

