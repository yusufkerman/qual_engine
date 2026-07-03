/**
 * @file sera_entity_registry.h
 * @brief Runtime Entity ID'leri ile Kalıcı UUID'ler arasındaki çift yönlü haritalama katmanı.
 */
#pragma once

#include <string>
#include <unordered_map>
#include "../ECS/entity.h" // Projedeki entity türünü dahil ediyoruz

namespace sera {

    class sera_entity_registry {
    public:
        sera_entity_registry() = default;
        ~sera_entity_registry() = default;

        // Haritaları tamamen temizler (Yeni sahneye geçerken çağrılacak)
        void clear();

        // Yeni bir eşleşme kaydeder
        void register_link(entity e, const std::string& uuid);

        // Canlı entity ID'sinden kalıcı UUID'yi bulur
        std::string get_uuid(entity e) const;

        // Kalıcı UUID'den canlı entity ID'sini bulur (Eğer sahne yüklenmişse)
        entity get_entity(const std::string& uuid) const;

        // Haritada bu entity var mı kontrolü
        bool has_entity(const std::string& uuid) const;
        bool has_uuid(entity e) const;

        // Rastgele ve benzersiz yeni bir UUID üretir (Entity'ler için)
        static std::string generate_uuid();

    private:
        std::unordered_map<entity, std::string> m_entity_to_uuid;
        std::unordered_map<std::string, entity> m_uuid_to_entity;
    };

} // namespace sera