#pragma once

/**
 * @file sera_types.h
 * @brief SERA modülünün ortak veri tipleri, hata raporlama ve doğrulama yapıları.
 */

#include <string>
#include <vector>

namespace sera {

    /**
     * @enum message_severity
     * @brief SERA doğrulama ve serileştirme mesajlarının önem derecesi.
     */
    enum class message_severity
    {
        info,       ///< Bilgilendirme mesajı.
        warning,    ///< İşlemin devamını engellemeyen kurtarılabilir durum uyarısı.
        critical    ///< Operasyonun başarısız olmasına yol açan kritik hata.
    };

    /**
     * @struct validation_message
     * @brief Serileştirme veya yükleme sürecinde üretilen tekil durum/hata kaydı.
     */
    struct validation_message
    {
        message_severity severity;     ///< Mesajın önem derecesi.
        std::string entity_name;       ///< İlgili varlığın adı (biliniyorsa).
        std::string component_name;    ///< İlgili bileşenin tür adı (biliniyorsa).
        std::string message;           ///< Detaylı durum veya hata açıklaması.
    };

    /**
     * @struct validation_report
     * @brief Kaydetme ve yükleme işlemlerinin genel durumunu ve üretilen tüm mesajları barındıran rapor nesnesi.
     */
    struct validation_report
    {
        bool success{ true };                      ///< İşlemin başarı durumu (Kritik hata eklendiğinde false olur).
        std::vector<validation_message> messages;  ///< İşlem sürecinde toplanan mesaj listesi.

        /**
         * @brief Rapora bilgilendirme mesajı ekler.
         */
        void add_info(const std::string& msg, const std::string& ent = "", const std::string& comp = "")
        {
            messages.push_back({ message_severity::info, ent, comp, msg });
        }

        /**
         * @brief Rapora uyarı mesajı ekler.
         */
        void add_warning(const std::string& msg, const std::string& ent = "", const std::string& comp = "")
        {
            messages.push_back({ message_severity::warning, ent, comp, msg });
        }

        /**
         * @brief Rapora kritik hata ekler ve başarı bayrağını false yapar.
         */
        void add_critical(const std::string& msg, const std::string& ent = "", const std::string& comp = "")
        {
            messages.push_back({ message_severity::critical, ent, comp, msg });
            success = false;
        }

        /**
         * @brief Raporda en az bir kritik hata bulunup bulunmadığını doğrular.
         */
        [[nodiscard]] bool has_critical_errors() const
        {
            for (const auto& msg : messages)
            {
                if (msg.severity == message_severity::critical)
                {
                    return true;
                }
            }
            return false;
        }
    };

} // namespace sera