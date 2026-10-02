#pragma once

/**
 * @file sera_entity_registry.h
 * @brief Çalışma zamanı (runtime) varlık kimlikleri ile kalıcı kimlikler (UUID) arasındaki çift yönlü eşleme katmanı.
 */

#include <string>
#include <unordered_map>
#include "../ECS/entity.h"

namespace sera {

    /**
     * @class sera_entity_registry
     * @brief Sahnedeki geçici (runtime) ECS varlık kimliklerini diskte saklanabilir kalıcı (persistent) UUID'lerle eşleştirir.
     */
    class sera_entity_registry
    {
    public:
        sera_entity_registry() = default;
        ~sera_entity_registry() = default;

        /**
         * @brief Eşleme tablolarını tamamen temizler (Sahne sıfırlamalarında kullanılır).
         */
        void clear();

        /**
         * @brief Çalışma zamanı varlık kimliği ile kalıcı UUID arasında yeni bir bağ kaydeder.
         * @param e Çalışma zamanı varlık kimliği (Entity ID).
         * @param uuid Kalıcı evrensel benzersiz kimlik (UUID).
         */
        void register_link(entity e, const std::string& uuid);

        /**
         * @brief Varlık kimliğine karşılık gelen kalıcı UUID'yi döndürür.
         * @param e Aranacak varlık kimliği.
         * @return Bulunursa UUID dizgesi, aksi halde boş dizge.
         */
        [[nodiscard]] std::string get_uuid(entity e) const;

        /**
         * @brief Kalıcı UUID'ye karşılık gelen çalışma zamanı varlık kimliğini döndürür.
         * @param uuid Aranacak kalıcı kimlik.
         * @return Bulunursa varlık kimliği, aksi halde 0 (geçersiz varlık).
         */
        [[nodiscard]] entity get_entity(const std::string& uuid) const;

        /**
         * @brief Belirtilen UUID'nin kayıtlı olup olmadığını kontrol eder.
         */
        [[nodiscard]] bool has_entity(const std::string& uuid) const;

        /**
         * @brief Belirtilen varlık kimliğinin kayıtlı olup olmadığını kontrol eder.
         */
        [[nodiscard]] bool has_uuid(entity e) const;

        /**
         * @brief Rastgele ve benzersiz yeni bir UUID v4 dizgesi (string) üretir.
         * @return Üretilen evrensel benzersiz kimlik dizgesi.
         */
        [[nodiscard]] static std::string generate_uuid();

    private:
        std::unordered_map<entity, std::string> m_entity_to_uuid; ///< Entity ID -> UUID eşleme tablosu.
        std::unordered_map<std::string, entity> m_uuid_to_entity; ///< UUID -> Entity ID eşleme tablosu.
    };

} // namespace sera