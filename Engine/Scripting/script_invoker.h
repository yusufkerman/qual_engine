#pragma once

/**
 * @file script_invoker.h
 * @brief Dinamik C++ kullanıcı betikleri için Win32 SEH tabanlı çökme kalkanı ve karantina yöneticisi.
 *
 * @details Bu sınıf, GameModule.dll içindeki kullanıcı betiklerinin (scriptable_entity)
 * on_create, on_update, on_fixed_update ve on_destroy çağrılarını donanımsal istisna
 * filtreleri (Win32 Structured Exception Handling) ile sarmalar.
 * 
 * Bir betik nullptr dereference (0xC0000005) veya sıfıra bölme (0xC0000094) gibi
 * ölümcül bir hata yaptığında editörün çökmesi engellenir; hatalı betik izole edilerek
 * karantinaya alınır ve ayrıntılı tanı bilgisi kaydedilir.
 */

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <malloc.h> // _resetstkoflw için
#endif

#include <string_view>
#include <string>
#include <cstdint>
#include <functional>
#include <iostream>
#include <format>
#include <utility>
#include "../Core/Logger/logger.h"

namespace qual::scripting {

/**
 * @struct script_exception_info
 * @brief Yakalanan donanım istisnasına dair ayrıntılı teknik tanı verileri.
 */
struct script_exception_info {
    uint32_t exception_code = 0;
    void* exception_address = nullptr;
    const char* exception_name = "Bilinmeyen Istisna";
    const char* exception_description = "Ayrintili bilgi yok";
    bool is_recoverable = false;
};

namespace detail {

#ifdef _WIN32
/**
 * @brief İstisna filtresi: İstisna kodunu inceler, debugger bildirimlerini ayıklar ve kurtarılabilirliğe karar verir.
 *
 * @param ep Windows istisna işaretçileri (GetExceptionInformation).
 * @param out_info Tanı yapısı.
 * @return int EXCEPTION_EXECUTE_HANDLER veya EXCEPTION_CONTINUE_SEARCH.
 */
inline int filter_seh_exception(EXCEPTION_POINTERS* ep, script_exception_info& out_info) noexcept {
    if (!ep || !ep->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const DWORD code = ep->ExceptionRecord->ExceptionCode;
    out_info.exception_code = static_cast<uint32_t>(code);
    out_info.exception_address = ep->ExceptionRecord->ExceptionAddress;

    // 1. Debugger ve CRT Dahili Bildirimleri (ASLA YUTMA!)
    // 0x406D1388: MSVC SetThreadName istisnası
    // 0x40010006: OutputDebugStringA bildirimi
    // 0xE06D7363: Standart C++ throw/catch istisnası (C++ runtime çözer)
    if (code == 0x406D1388 || code == 0x40010006 || code == 0xE06D7363) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // 2. Kurtarılabilir Donanım Hatalarını Sınıflandır
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            out_info.exception_name = "EXCEPTION_ACCESS_VIOLATION (0xC0000005)";
            out_info.exception_description = "Gecersiz bellek bolgesine erisim veya nullptr dereference.";
            out_info.is_recoverable = true;
            return EXCEPTION_EXECUTE_HANDLER;

        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            out_info.exception_name = "EXCEPTION_INT_DIVIDE_BY_ZERO (0xC0000094)";
            out_info.exception_description = "Tamsayi sifira bolme hatasi.";
            out_info.is_recoverable = true;
            return EXCEPTION_EXECUTE_HANDLER;

        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            out_info.exception_name = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED (0xC000008C)";
            out_info.exception_description = "Donanimsal dizi sinir asimi.";
            out_info.is_recoverable = true;
            return EXCEPTION_EXECUTE_HANDLER;

        case EXCEPTION_ILLEGAL_INSTRUCTION:
            out_info.exception_name = "EXCEPTION_ILLEGAL_INSTRUCTION (0xC000001D)";
            out_info.exception_description = "Gecersiz veya bozulmus islemci makine komutu.";
            out_info.is_recoverable = true;
            return EXCEPTION_EXECUTE_HANDLER;

        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            out_info.exception_name = "EXCEPTION_FLT_DIVIDE_BY_ZERO (0xC000008E)";
            out_info.exception_description = "Kayan noktali sifira bolme hatasi.";
            out_info.is_recoverable = true;
            return EXCEPTION_EXECUTE_HANDLER;

        case EXCEPTION_FLT_INVALID_OPERATION:
            out_info.exception_name = "EXCEPTION_FLT_INVALID_OPERATION (0xC0000090)";
            out_info.exception_description = "Gecersiz kayan nokta matematik islemi.";
            out_info.is_recoverable = true;
            return EXCEPTION_EXECUTE_HANDLER;

        case EXCEPTION_STACK_OVERFLOW:
            // Yığın taşması durumunda işletim sistemi koruma sayfasını (guard page) tüketmiştir.
            // Sürecin anında ölmemesi için _resetstkoflw çağrılmalı ancak devam etmek son derece risklidir.
            _resetstkoflw();
            out_info.exception_name = "EXCEPTION_STACK_OVERFLOW (0xC00000FD)";
            out_info.exception_description = "Yigin tasmasi (Sonsuz ozyineleme / recursive call). Olumcul durum.";
            out_info.is_recoverable = false;
            return EXCEPTION_EXECUTE_HANDLER;

        default:
            // Bilinmeyen veya kurtarılamaz sistem istisnalarını dışarı bırak
            return EXCEPTION_CONTINUE_SEARCH;
    }
}

/**
 * @brief C2712 hatasını önlemek için C++ yıkıcısı olmayan saf fonksiyon göstericili SEH sarmalayıcısı.
 *
 * @note Bu fonksiyon çerçevesinde (stack frame) hiçbir C++ nesnesi bulunmaz.
 */
typedef void (*raw_invoker_fn)(void* user_data);

inline bool execute_seh_boundary(raw_invoker_fn fn, void* user_data, script_exception_info& out_info) noexcept {
    __try {
        fn(user_data);
        return true;
    }
    __except (filter_seh_exception(GetExceptionInformation(), out_info)) {
        return false;
    }
}
#endif

} // namespace detail

/**
 * @class script_invoker
 * @brief Güvenli betik yürütme arayüzü ve karantina yöneticisi.
 */
class script_invoker {
public:
    /// @brief Çökme kalkanı aktif mi? (Editör ayarlarından açılıp kapatılabilir)
    inline static bool s_boundary_enabled = true;

    /// @brief Debugger (Visual Studio / RemedyBG) bağlıyken hatayı yakalayıp yutmak yerine debugger'a bırak.
    inline static bool s_pass_through_when_debugging = true;

    /// @brief Motor oturumu boyunca engellenen toplam çökme sayısı (Telemetri).
    inline static uint32_t s_crashes_prevented_count = 0;

    /**
     * @brief Bir kullanıcı betik fonksiyonunu güvenli çökme kalkanı altında yürütür.
     *
     * @tparam Func Yürütülecek fonksiyon veya lambda.
     * @param script_name Betiğin sınıf adı (Örn: "RotatorScript").
     * @param entity_id Betiğin bağlı olduğu varlık kimliği.
     * @param stage Yaşam döngüsü evresi ("on_create", "on_update", "on_fixed_update", "on_destroy").
     * @param func Yürütülecek çağrı gövdesi.
     * @param out_error Hata durumunda biçimlendirilmiş açıklamanın yazılacağı isteğe bağlı string göstericisi.
     * @return true Betik başarıyla ve hatasız çalıştıysa.
     * @return false Donanımsal bir çökme yakalandı ve engellendiyse.
     */
    template <typename Func>
    static bool invoke(
        std::string_view script_name,
        uint32_t entity_id,
        std::string_view stage,
        Func&& func,
        std::string* out_error = nullptr)
    {
#ifdef _WIN32
        // 1. Kalkan kapalıysa veya Debugger bağlıyken debugger'a bırakma seçeneği aktifse doğrudan çağır
        if (!s_boundary_enabled || (s_pass_through_when_debugging && IsDebuggerPresent())) {
            func();
            return true;
        }

        // 2. Lambda'yı nesne yıkıcısı içermeyen ham fonksiyona eşle
        auto raw_lambda = [](void* ctx) {
            auto* target = static_cast<std::decay_t<Func>*>(ctx);
            (*target)();
        };

        script_exception_info info{};
        bool success = detail::execute_seh_boundary(raw_lambda, &func, info);

        if (!success) {
            ++s_crashes_prevented_count;

            // Merkezi Logger'a varlık kimliği ile kaydet (Vesta Konsol Panelinde görüntülenir)
            QLOG_SCRIPT_ERROR(entity_id,
                "[CRASH ENGELLENDI] Varlik: {} | Betik: '{}' | Evre: '{}' | Hata: {} ({}) at {:p}",
                entity_id,
                script_name,
                stage,
                info.exception_name,
                info.exception_description,
                info.exception_address
            );

            // 3. Ayrıntılı hata tanısı oluştur (C++20 std::format ile güvenli biçimlendirme)
            std::string formatted_error = std::format(
                "[CRASH ENGELLENDI] Varlik ID: {} | Betik: '{}' | Evre: '{}' | Hata: {} at Adres: {:p} ({})",
                entity_id,
                script_name,
                stage,
                info.exception_name,
                info.exception_address,
                info.exception_description
            );

            // Standart hata akışına belirgin kırmızı tanı başlığı dök
            std::cerr << "\n"
                      << "================================================================================\n"
                      << " [QUALENGINE BETIK COKME KALKANI / SCRIPT CRASH BOUNDARY]\n"
                      << " " << formatted_error << "\n"
                      << " Eylem: Betik karantinaya alindi (is_quarantined = true, is_enabled = false).\n"
                      << " Editorun kapanmasi engellendi. Veri kaybi onlendi.\n"
                      << "================================================================================\n\n";

            if (out_error) {
                *out_error = std::move(formatted_error);
            }

            return false;
        }

        return true;
#else
        // POSIX / Non-Windows fallback
        func();
        return true;
#endif
    }
};

} // namespace qual::scripting

