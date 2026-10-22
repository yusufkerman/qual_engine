#pragma once
#include "entity.h"

class registry;

struct component_pool_interface
{
    virtual ~component_pool_interface() = default;

    virtual void clone_entity(entity e,
        registry& dst,
        const registry& src) const = 0;
};