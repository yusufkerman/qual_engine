/**
 * @file sera_types.h
 * @brief SERA modülünün ortak veri tipleri, hata raporlama ve durum yönetim yapıları.
 */
#pragma once

#include <string>
#include <vector>

namespace sera {

    // SERA içindeki işlemlerin ciddiyet seviyesi
    enum class message_severity {
        info,       // Bilgilendirme (Örn: Sahne başarıyla yüklendi)
        warning,    // Uyarı (Örn: Bir komponent bozuktu, varsayılan değer atandı ama sahne açıldı)
        critical    // Kritik Hata (Örn: JSON dosyası tamamen bozuk, sahne hiç okunamadı)
    };

    // Tek bir hata veya durum kaydı
    struct validation_message {
        message_severity severity;
        std::string entity_name; // Hatanın hangi entity'de çıktığı (Eğer biliniyorsa)
        std::string component_name; // Hatanın hangi komponentte çıktığı (Eğer biliniyorsa)
        std::string message;        // Detaylı açıklama metni
    };

    // Tüm Save/Load operasyonlarının sonucunu taşıyan rapor nesnesi
    struct validation_report {
        bool success{ true }; // İşlem genel olarak başarılı mı? (Uyarılar olsa bile true kalabilir)
        std::vector<validation_message> messages;

        // Yardımcı fonksiyonlar: Rapora hızlıca log eklemek için
        void add_info(const std::string& msg, const std::string& ent = "", const std::string& comp = "") {
            messages.push_back({ message_severity::info, ent, comp, msg });
        }

        void add_warning(const std::string& msg, const std::string& ent = "", const std::string& comp = "") {
            messages.push_back({ message_severity::warning, ent, comp, msg });
        }

        void add_critical(const std::string& msg, const std::string& ent = "", const std::string& comp = "") {
            messages.push_back({ message_severity::critical, ent, comp, msg });
            success = false; // Kritik bir hata varsa operasyon tamamen başarısız sayılır
        }

        // Raporda kritik hata var mı kontrolü
        bool has_critical_errors() const {
            for (const auto& msg : messages) {
                if (msg.severity == message_severity::critical) return true;
            }
            return false;
        }
    };

} // namespace sera