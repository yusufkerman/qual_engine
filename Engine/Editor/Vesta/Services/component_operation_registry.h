#pragma once

#include <functional>
#include <unordered_map>

#include "../../ECS/entity.h"

// Forward declaration
class registry;

namespace vesta
{

    using ComponentAddFn =
        std::function<void(entity, registry&)>;

    using ComponentRemoveFn =
        std::function<void(entity, registry&)>;

    using ComponentHasFn =
        std::function<bool(entity, registry&)>;

    struct ComponentOperationInfo
    {
        ComponentAddFn add_fn;
        ComponentRemoveFn remove_fn;
        ComponentHasFn has_fn;
    };

    class component_operation_registry
    {
    public:

        static void register_component(
            size_t hash,
            const ComponentOperationInfo& info)
        {
            get_registry_internal()[hash] = info;
        }

        static const ComponentOperationInfo*
            find(size_t hash)
        {
            auto& registry = get_registry_internal();

            auto it = registry.find(hash);

            if (it == registry.end())
                return nullptr;

            return &it->second;
        }

        static const std::unordered_map<size_t, ComponentOperationInfo>&
            get_registry()
        {
            return get_registry_internal();
        }

    private:

        static std::unordered_map<size_t, ComponentOperationInfo>&
            get_registry_internal()
        {
            static std::unordered_map<size_t, ComponentOperationInfo> registry;

            return registry;
        }
    };

}