#pragma once

/**
 * @file component_pool_legacy.h
 * @brief Eski prototip bileşen havuzu sınıfı.
 */

#include <unordered_map>
#include "component_pool_interface_legacy.h"
#include "registry_legacy.h"

namespace qual::legacy {

template<typename T>
class component_pool : public component_pool_interface
{
public:
    std::unordered_map<entity, T> data;

    virtual void clone_entity(entity e, registry& dst, const registry& src) const override
    {
        if (const auto* comp = src.get_component<T>(e))
        {
            dst.add_component<T>(e, *comp);
        }
    }
};

} // namespace qual::legacy

