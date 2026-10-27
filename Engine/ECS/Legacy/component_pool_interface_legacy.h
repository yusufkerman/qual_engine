#pragma once

/**
 * @file component_pool_interface_legacy.h
 * @brief Eski prototip bileşen havuzları soyut arayüzü.
 */

#include "../entity.h"

namespace qual::legacy {

class registry;

struct component_pool_interface
{
    virtual ~component_pool_interface() = default;
    virtual void clone_entity(entity e, registry& dst, const registry& src) const = 0;
};

} // namespace qual::legacy

