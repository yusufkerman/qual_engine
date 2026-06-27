#pragma once
#include <unordered_map>
#include <functional>
#include "../../ECS/entity.h"

// Forward declarations - Sıfır bağımlılık
class registry;
struct EngineContext;
struct EditorContext;

using ComponentRenderFn = std::function<void(entity, registry&, EngineContext&, EditorContext&)>;
using ComponentAddFn = std::function<void(entity, registry&, EngineContext&, EditorContext&)>;
using ComponentHasFn = std::function<bool(entity, const registry&)>;

struct ComponentUIInfo {
    const char* name;
    ComponentRenderFn render_fn;
    ComponentAddFn add_fn;
    ComponentHasFn has_fn;
};

class component_ui_registry {
public:
    // Şablon yerine doğrudan struct nesnesini kabul ediyoruz
    static void register_component(size_t type_hash, const ComponentUIInfo& info) {
        get_components()[type_hash] = info;
    }

    static const std::unordered_map<size_t, ComponentUIInfo>& get_registry() {
        return get_components();
    }

private:
    static std::unordered_map<size_t, ComponentUIInfo>& get_components() {
        static std::unordered_map<size_t, ComponentUIInfo> components;
        return components;
    }
};