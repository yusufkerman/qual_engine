#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <type_traits>
#include <utility>

#include "entity.h"
#include "components.h"

class registry {
public:
    entity create_entity();
    void add_entity(entity e);
    void remove_entity(entity e);

    const std::unordered_set<entity>& get_all_entities() const;

    // =========================
    // COMPONENT API
    // =========================
    template<typename T, typename... Args>
    T& add_component(entity e, Args&&... args)
    {
        auto& p = pool<T>();
        return p[e] = T{ std::forward<Args>(args)... };
    }

    template<typename T>
    T* get_component(entity e)
    {
        auto& p = pool<T>();
        auto it = p.find(e);
        return it != p.end() ? &it->second : nullptr;
    }

    template<typename T>
    const T* get_component(entity e) const
    {
        const auto& p = pool<T>();
        auto it = p.find(e);
        return it != p.end() ? &it->second : nullptr;
    }

    template<typename T>
    bool has_component(entity e) const
    {
        const auto& p = pool<T>();
        return p.find(e) != p.end();
    }

    template<typename T>
    void remove_component(entity e)
    {
        pool<T>().erase(e);
    }

    template<typename T>
    auto& get_all()
    {
        return pool<T>();
    }

    template<typename T>
    const auto& get_all() const
    {
        return pool<T>();
    }

    // =========================
    // CLONE SYSTEM
    // =========================
    void clone_entity(entity e, registry& dst) const;

private:
    entity next_entity = 1;

    std::unordered_set<entity> entities;

    std::unordered_map<entity, transform_component> transforms;
    std::unordered_map<entity, mesh_component> meshes;
    std::unordered_map<entity, camera_component> cameras;
    std::unordered_map<entity, light_component> lights;
    std::unordered_map<entity, name_component> names;

    template<typename T>
    std::unordered_map<entity, T>& pool();

    template<typename T>
    const std::unordered_map<entity, T>& pool() const;
};

// =========================
// POOL DEFINITIONS
// =========================
template<typename T>
std::unordered_map<entity, T>& registry::pool()
{
    if constexpr (std::is_same_v<T, transform_component>) return transforms;
    if constexpr (std::is_same_v<T, mesh_component>) return meshes;
    if constexpr (std::is_same_v<T, camera_component>) return cameras;
    if constexpr (std::is_same_v<T, light_component>) return lights;
    if constexpr (std::is_same_v<T, name_component>) return names;

    static std::unordered_map<entity, T> dummy;
    return dummy;
}

template<typename T>
const std::unordered_map<entity, T>& registry::pool() const
{
    if constexpr (std::is_same_v<T, transform_component>) return transforms;
    if constexpr (std::is_same_v<T, mesh_component>) return meshes;
    if constexpr (std::is_same_v<T, camera_component>) return cameras;
    if constexpr (std::is_same_v<T, light_component>) return lights;
    if constexpr (std::is_same_v<T, name_component>) return names;

    static std::unordered_map<entity, T> dummy;
    return dummy;
}