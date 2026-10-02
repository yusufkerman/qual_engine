/**
 * @file sera_entity_registry.cpp
 * @brief Varlık kimliği ve UUID eşleştirme ile evrensel kimlik üretme mantığının uygulaması.
 */

#include "sera_entity_registry.h"
#include "../Externals/uuid/uuid.h"
#include <random>

namespace sera {

    void sera_entity_registry::clear()
    {
        m_entity_to_uuid.clear();
        m_uuid_to_entity.clear();
    }

    void sera_entity_registry::register_link(entity e, const std::string& uuid)
    {
        m_entity_to_uuid[e] = uuid;
        m_uuid_to_entity[uuid] = e;
    }

    std::string sera_entity_registry::get_uuid(entity e) const
    {
        const auto it = m_entity_to_uuid.find(e);
        if (it != m_entity_to_uuid.end())
        {
            return it->second;
        }
        return "";
    }

    entity sera_entity_registry::get_entity(const std::string& uuid) const
    {
        const auto it = m_uuid_to_entity.find(uuid);
        if (it != m_uuid_to_entity.end())
        {
            return it->second;
        }
        return 0;
    }

    bool sera_entity_registry::has_entity(const std::string& uuid) const
    {
        return m_uuid_to_entity.find(uuid) != m_uuid_to_entity.end();
    }

    bool sera_entity_registry::has_uuid(entity e) const
    {
        return m_entity_to_uuid.find(e) != m_entity_to_uuid.end();
    }

    std::string sera_entity_registry::generate_uuid()
    {
        static std::random_device rd;
        static std::mt19937 generator(rd());
        static uuids::uuid_random_generator gen(generator);

        const uuids::uuid id = gen();
        return uuids::to_string(id);
    }

} // namespace sera