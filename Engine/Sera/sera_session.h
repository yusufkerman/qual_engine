#pragma once

/**
 * @file sera_session.h
 * @brief SERA sisteminin sahne oturum yönetimini, I/O süreçlerini ve varlık kaydını koordine eden ana sınıf.
 */

#include "sera_types.h"
#include "sera_entity_registry.h"
#include "sera_asset_resolver.h"
#include "../Scene/scene.h"
#include <string>

struct EngineContext;

namespace sera {

    /**
     * @class sera_session
     * @brief Sahne verilerinin kalıcı depolama formatına dönüştürülmesini ve bellek dünyasına yeniden yüklenmesini yöneten oturum nesnesi.
     */
    class sera_session
    {
    public:
        /**
         * @brief Oturumu motor bağlamı ile ilklendirir.
         * @param ctx Varlık çözümleyici ve motor alt sistemleri için gerekli bağlam referansı.
         */
        explicit sera_session(EngineContext& ctx);
        ~sera_session() = default;

        // Kopyalama semantiği devre dışı bırakılmıştır
        sera_session(const sera_session&) = delete;
        sera_session& operator=(const sera_session&) = delete;

        /**
         * @brief Aktif sahneyi JSON formatında belirtilen dosya yoluna serileştirir.
         * @param filepath Kaydedilecek dosyanın tam veya göreceli yolu.
         * @param current_scene Kaynak sahne referansı.
         * @return İşlem adımlarını ve başarı durumunu içeren rapor nesnesi.
         */
        validation_report save(const std::string& filepath, const scene& current_scene);

        /**
         * @brief Diskteki JSON dosyasından sahneyi ve UUID eşleme tablolarını sıfırdan yeniden kurar.
         * @param filepath Okunacak dosyanın yolu.
         * @param out_scene Verilerin aktarılacağı hedef sahne nesnesi.
         * @return Yükleme sürecini ve olası uyarı/hataları içeren rapor nesnesi.
         */
        validation_report load(const std::string& filepath, scene& out_scene);

        /**
         * @brief Varlık-UUID eşleme defterine erişim sağlar.
         */
        [[nodiscard]] sera_entity_registry& get_entity_registry() noexcept { return m_entity_registry; }

        /**
         * @brief Varlık-UUID eşleme defterine salt okunur erişim sağlar.
         */
        [[nodiscard]] const sera_entity_registry& get_entity_registry() const noexcept { return m_entity_registry; }

    private:
        EngineContext& m_ctx;                    ///< Motor bağlamı referansı.
        sera_entity_registry m_entity_registry;  ///< Sahne içi Entity ID - UUID eşleme yöneticisi.
        sera_asset_resolver m_asset_resolver;    ///< ARIA Handle - GUID soyutlama köprüsü.
    };

} // namespace sera