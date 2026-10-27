/**
 * @file crash_handler.cpp
 * @brief QualEngine post-mortem çökme yakalama ve Minidump (.dmp) üretim uygulaması.
 */

#include "crash_handler.h"
#include "../Logger/logger.h"

#ifdef _WIN32
#include <dbghelp.h>
#include <csignal>
#include <exception>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "dbghelp.lib")

namespace qual::core {

void crash_handler::format_timestamp(char* out_buf, size_t buf_size)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::snprintf(out_buf, buf_size, "%04d%02d%02d_%02d%02d%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

bool crash_handler::write_minidump(void* ep, const char* reason)
{
    // Hedef klasörün varlığını güvenceye al
    CreateDirectoryA(s_crash_dump_dir, NULL);

    char time_str[32] = { 0 };
    format_timestamp(time_str, sizeof(time_str));

    // Sıfır heap tahsisi: Yol dizgilerini yığın (stack) üzerinde oluştur
    char dump_path[MAX_PATH] = { 0 };
    std::snprintf(dump_path, sizeof(dump_path), "%s/QualEngine_Crash_%s.dmp", s_crash_dump_dir, time_str);

    char report_path[MAX_PATH] = { 0 };
    std::snprintf(report_path, sizeof(report_path), "%s/QualEngine_Crash_%s_Report.txt", s_crash_dump_dir, time_str);

    // 1. Minidump (.dmp) Dosyasını Oluştur ve Yaz
    HANDLE hFile = CreateFileA(
        dump_path,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    bool dump_success = false;
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei{};
        MINIDUMP_EXCEPTION_INFORMATION* pMei = nullptr;

        if (ep) {
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = static_cast<EXCEPTION_POINTERS*>(ep);
            mei.ClientPointers = FALSE;
            pMei = &mei;
        }

        // Kapsamlı post-mortem analizi için thread ve handle bilgileri dahil edilir
        const auto dump_type = static_cast<MINIDUMP_TYPE>(
            MiniDumpNormal |
            MiniDumpWithHandleData |
            MiniDumpWithThreadInfo |
            MiniDumpWithUnloadedModules
        );

        dump_success = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            dump_type,
            pMei,
            nullptr,
            nullptr
        ) == TRUE;

        CloseHandle(hFile);
    }

    // 2. İnsan Tarafından Okunabilir Metin Raporu (.txt) Oluştur
    HANDLE hReport = CreateFileA(
        report_path,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hReport != INVALID_HANDLE_VALUE) {
        char buffer[1024] = { 0 };
        int len = std::snprintf(buffer, sizeof(buffer),
            "================================================================================\r\n"
            "                   QUALENGINE POST-MORTEM COKME RAPORU                         \r\n"
            "================================================================================\r\n"
            "Zaman Damgasi     : %s\r\n"
            "Cokme Nedeni      : %s\r\n"
            "Process ID (PID)  : %lu\r\n"
            "Thread ID (TID)   : %lu\r\n"
            "Minidump Dosyasi  : %s (%s)\r\n"
            "================================================================================\r\n"
            "Hata Ayiklama Rehberi:\r\n"
            "  1. Visual Studio veya WinDbg ile '%s' dosyasini acin.\r\n"
            "  2. 'Debug with Native Only' secenegini secin.\r\n"
            "  3. Cokme anindaki kaynak kod satiri, yigin izi (callstack) ve yerel degiskenler\r\n"
            "     otomatik olarak incelenebilir.\r\n"
            "================================================================================\r\n",
            time_str,
            reason ? reason : "Bilinmeyen Hata",
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            dump_path,
            dump_success ? "Basariyla Yazildi" : "Yazma Basarisiz",
            dump_path
        );

        if (len > 0) {
            DWORD written = 0;
            WriteFile(hReport, buffer, static_cast<DWORD>(len), &written, NULL);
        }
        CloseHandle(hReport);
    }

    // 3. Standart Hata Çıktısına Belirgin Çökme Başlığı Dök
    std::fprintf(stderr, "\n"
        "================================================================================\n"
        " [QUALENGINE KRITIK COKME / FATAL SYSTEM CRASH]\n"
        " Neden: %s\n"
        " Minidump Olusturuldu : %s\n"
        " Aydinlatici Rapor    : %s\n"
        " Surec guvenli sekilde sonlandiriliyor.\n"
        "================================================================================\n\n",
        reason ? reason : "Bilinmeyen Hata",
        dump_path,
        report_path
    );

    return dump_success;
}

LONG WINAPI crash_handler::unhandled_exception_filter(EXCEPTION_POINTERS* ep)
{
    // Debugger bağlıysa hatayı doğrudan debugger'a bırak
    if (s_pass_through_debugger && IsDebuggerPresent()) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    char reason[128] = { 0 };
    const DWORD code = (ep && ep->ExceptionRecord) ? ep->ExceptionRecord->ExceptionCode : 0;
    std::snprintf(reason, sizeof(reason), "SEH Islenmeyen Donanimsal Istisna (Kod: 0x%08X)", code);

    write_minidump(ep, reason);
    return EXCEPTION_EXECUTE_HANDLER;
}

void crash_handler::invalid_parameter_handler(
    const wchar_t* /*expression*/,
    const wchar_t* /*function*/,
    const wchar_t* /*file*/,
    unsigned int line,
    uintptr_t /*reserved*/)
{
    if (s_pass_through_debugger && IsDebuggerPresent()) {
        __debugbreak();
    }

    char reason[128] = { 0 };
    std::snprintf(reason, sizeof(reason), "CRT Gecersiz Parametre Hatasi (Satir: %u)", line);
    write_minidump(nullptr, reason);
    ExitProcess(1);
}

void crash_handler::purecall_handler()
{
    if (s_pass_through_debugger && IsDebuggerPresent()) {
        __debugbreak();
    }

    write_minidump(nullptr, "CRT Saf Sanal Fonksiyon Cagrisi (Pure Virtual Function Call)");
    ExitProcess(1);
}

void crash_handler::terminate_handler()
{
    if (s_pass_through_debugger && IsDebuggerPresent()) {
        __debugbreak();
    }

    write_minidump(nullptr, "C++ std::terminate() Tetiklendi (Yakalanmayan C++ Istisnasi)");
    _Exit(1);
}

void crash_handler::abort_signal_handler(int /*signal*/)
{
    if (s_pass_through_debugger && IsDebuggerPresent()) {
        __debugbreak();
    }

    write_minidump(nullptr, "CRT abort() veya SIGABRT Sinyali Alindi");
    _Exit(1);
}

void crash_handler::init()
{
    // Çökme klasörünü hazırla
    CreateDirectoryA(s_crash_dump_dir, NULL);

    // 1. İşlenmeyen SEH İstisnalarını Yakala
    SetUnhandledExceptionFilter(unhandled_exception_filter);

    // 2. CRT Geçersiz Parametre Kancası
    _set_invalid_parameter_handler(invalid_parameter_handler);

    // 3. CRT Saf Sanal Fonksiyon Kancası
    _set_purecall_handler(purecall_handler);

    // 4. C++ std::terminate Kancası
    std::set_terminate(terminate_handler);

    // 5. CRT abort() Sinyal Kancası
    std::signal(SIGABRT, abort_signal_handler);

    // CRT abort dialogunun arayüzü dondurmasını engelle, doğrudan sinyale yönlendir
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

    QLOG_INFO(core, "QualEngine Cokme Kancalari ve Minidump Mimarisi (Faz 5) basariyla aktiflestirildi.");
}

} // namespace qual::core

#else // Non-Windows Stub

namespace qual::core {
void crash_handler::init() {}
bool crash_handler::write_minidump(void*, const char*) { return false; }
} // namespace qual::core

#endif

