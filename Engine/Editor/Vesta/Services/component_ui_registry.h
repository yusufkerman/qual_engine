#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include "../../ECS/entity.h"

class registry;

namespace vesta
{
    // Eski EngineContext ve EditorContext tamamen kaldırıldı! SAF VESTA!
    using ComponentRenderFn = std::function<void(entity, registry&)>;

    struct ComponentUIInfo
    {
        std::string display_name;
        ComponentRenderFn render_fn;
    };

    class component_ui_registry
    {
    public:
        static void register_component(size_t hash, const ComponentUIInfo& info)
        {
            get_components()[hash] = info;
        }

        static const ComponentUIInfo* find(size_t hash)
        {
            auto& registry = get_components();
            auto it = registry.find(hash);
            if (it == registry.end())
                return nullptr;
            return &it->second;
        }

        static const std::unordered_map<size_t, ComponentUIInfo>& get_registry()
        {
            return get_components();
        }

    private:
        static std::unordered_map<size_t, ComponentUIInfo>& get_components()
        {
            static std::unordered_map<size_t, ComponentUIInfo> components;
            return components;
        }
    };
}