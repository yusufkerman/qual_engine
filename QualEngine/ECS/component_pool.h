#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>

#include "component_pool_interface.h"
#include "registry.h"

template<typename T>
class component_pool : public component_pool_interface
{
public:
    std::unordered_map<entity, T> data;

    void clone_entity(entity e,
        registry& dst,
        const registry& src) const override
    {
        if (auto* comp = src.get_component<T>(e))
        {
            dst.add_component<T>(e, *comp);
        }
    }
};