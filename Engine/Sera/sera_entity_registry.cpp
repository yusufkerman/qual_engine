#include "sera_entity_registry.h"
#include "../Externals/uuid/uuid.h" // Projedeki mevcut UUID kütüphanesini kullanıyoruz

namespace sera {

    void sera_entity_registry::clear() {
        m_entity_to_uuid.clear();
        m_uuid_to_entity.clear();
    }

    void sera_entity_registry::register_link(entity e, const std::string& uuid) {
        m_entity_to_uuid[e] = uuid;
        m_uuid_to_entity[uuid] = e;
    }

    std::string sera_entity_registry::get_uuid(entity e) const {
        auto it = m_entity_to_uuid.find(e);
        if (it != m_entity_to_uuid.end()) {
            return it->second;
        }
        return "";
    }

    entity sera_entity_registry::get_entity(const std::string& uuid) const {
        auto it = m_uuid_to_entity.find(uuid);
        if (it != m_uuid_to_entity.end()) {
            return it->second;
        }
        return 0; // Geçersiz entity fallback (ECS yapına göre burayı null/0 entity yapabilirsin)
    }

    bool sera_entity_registry::has_entity(const std::string& uuid) const {
        return m_uuid_to_entity.find(uuid) != m_uuid_to_entity.end();
    }

    bool sera_entity_registry::has_uuid(entity e) const {
        return m_entity_to_uuid.find(e) != m_entity_to_uuid.end();
    }

    std::string sera_entity_registry::generate_uuid() {
        static std::random_device rd;
        static std::mt19937 generator(rd());

        // stduuid kütüphanesinin standart UUID v4 üreticisi
        static uuids::uuid_random_generator gen(generator);

        uuids::uuid const id = gen();
        return uuids::to_string(id); // Üretilen UUID nesnesini string'e çevirip dönüyoruz
    }

} // namespace sera