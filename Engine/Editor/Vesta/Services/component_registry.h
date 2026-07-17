#pragma once

#include <functional>
#include <unordered_map>

#include "../../ECS/entity.h"

class registry;

namespace vesta
{

    using ComponentAddFn =
        std::function<void(entity, registry&)>;

    using ComponentRemoveFn =
        std::function<void(entity, registry&)>;

    using ComponentHasFn =
        std::function<bool(entity, registry&)>;

    struct ComponentInfo
    {
        ComponentAddFn add_fn;
        ComponentRemoveFn remove_fn;
        ComponentHasFn has_fn;
    };

    class component_registry
    {
    public:

        static void register_component(
            size_t hash,
            const ComponentInfo& info)
        {
            get_components()[hash] = info;
        }

        static const ComponentInfo*
            find(size_t hash)
        {
            auto& registry = get_components();

            auto it = registry.find(hash);

            if (it == registry.end())
                return nullptr;

            return &it->second;
        }

    private:

        static std::unordered_map<size_t, ComponentInfo>&
            get_components()
        {
            static std::unordered_map<
                size_t,
                ComponentInfo> components;

            return components;
        }
    };

}