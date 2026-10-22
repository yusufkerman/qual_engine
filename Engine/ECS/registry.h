#pragma once

/**
 * @file registry.h
 * @brief Varlıkları (Entity) ve bileşen havuzlarını (Component Pools) yöneten ECS veri tabanı.
 */

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <type_traits>
#include <utility>
#include "entity.h"
#include "components.h"

class registry
{
public:
    registry() = default;
    ~registry() = default;

    /**
     * @brief Yeni ve benzersiz bir varlık (Entity) oluşturur.
     */
    entity create_entity();

    /**
     * @brief Var olan bir varlık kimliğini sisteme ekler (Örn: Serileştirme sırasında).
     */
    void add_entity(entity e);

    /**
     * @brief Varlığı ve ona bağlı olan tüm bileşenleri bellekten siler.
     */
    void remove_entity(entity e);

    /**
     * @brief Kayıtlı tüm varlıkların kümesini döner.
     */
    [[nodiscard]] const std::unordered_set<entity>& get_all_entities() const;

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

    /**
     * @brief Belirtilen varlığı aynı kimlikle hedef kayıt defterine kopyalar (Örn: Scene play/stop geçişleri).
     */
    void clone_entity(entity e, registry& dst) const;

    /**
     * @brief Belirtilen varlığın yeni bir kimlikle tam bir kopyasını oluşturur.
     */
    entity duplicate_entity(entity source);

private:
    entity next_entity = 1; ///< Bir sonraki üretilecek varlık kimliği sayacı.

    std::unordered_set<entity> entities;

    std::unordered_map<entity, transform_component> transforms;
    std::unordered_map<entity, mesh_component> meshes;
    std::unordered_map<entity, camera_component> cameras;
    std::unordered_map<entity, light_component> lights;
    std::unordered_map<entity, name_component> names;
    std::unordered_map<entity, bounds_component> bounds;
    std::unordered_map<entity, material_component> materials;
    std::unordered_map<entity, rigidbody_component> rigidbodies;
    std::unordered_map<entity, collider_component> colliders;
    std::unordered_map<entity, native_script_component> scriptable_components;
    std::unordered_map<entity, hierarchy_component> hierarchies;
    std::unordered_map<entity, skinned_mesh_component> skinned_meshes;
    std::unordered_map<entity, skeleton_component> skeletons;
    std::unordered_map<entity, animator_component> animators;
    std::unordered_map<entity, bone_attachment_component> bone_attachments;
    std::unordered_map<entity, canvas_component> canvases;
    std::unordered_map<entity, rect_transform_component> rect_transforms;
    std::unordered_map<entity, ui_text_component> ui_texts;
    std::unordered_map<entity, ui_image_component> ui_images;
    std::unordered_map<entity, ui_button_component> ui_buttons;
    std::unordered_map<entity, audio_source_component> audio_sources;
    std::unordered_map<entity, audio_listener_component> audio_listeners;

    template<typename T>
    std::unordered_map<entity, T>& pool();

    template<typename T>
    const std::unordered_map<entity, T>& pool() const;

    /**
     * @brief Hedef varlığa kaynak varlığın bileşenlerinin kopyalarını aktarır.
     */
    void copy_components_internal(entity srcEntity, entity dstEntity, registry& dstRegistry) const;
};

// =========================
// POOL DEFINITIONS (INLINE)
// =========================

template<typename T>
inline std::unordered_map<entity, T>& registry::pool()
{
    if constexpr (std::is_same_v<T, transform_component>) return transforms;
    if constexpr (std::is_same_v<T, mesh_component>) return meshes;
    if constexpr (std::is_same_v<T, camera_component>) return cameras;
    if constexpr (std::is_same_v<T, light_component>) return lights;
    if constexpr (std::is_same_v<T, name_component>) return names;
    if constexpr (std::is_same_v<T, bounds_component>) return bounds;
    if constexpr (std::is_same_v<T, material_component>) return materials;
    if constexpr (std::is_same_v<T, rigidbody_component>) return rigidbodies;
    if constexpr (std::is_same_v<T, collider_component>) return colliders;
    if constexpr (std::is_same_v<T, native_script_component>) return scriptable_components;
    if constexpr (std::is_same_v<T, hierarchy_component>) return hierarchies;
    if constexpr (std::is_same_v<T, skinned_mesh_component>) return skinned_meshes;
    if constexpr (std::is_same_v<T, skeleton_component>) return skeletons;
    if constexpr (std::is_same_v<T, animator_component>) return animators;
    if constexpr (std::is_same_v<T, bone_attachment_component>) return bone_attachments;
    if constexpr (std::is_same_v<T, canvas_component>) return canvases;
    if constexpr (std::is_same_v<T, rect_transform_component>) return rect_transforms;
    if constexpr (std::is_same_v<T, ui_text_component>) return ui_texts;
    if constexpr (std::is_same_v<T, ui_image_component>) return ui_images;
    if constexpr (std::is_same_v<T, ui_button_component>) return ui_buttons;
    if constexpr (std::is_same_v<T, audio_source_component>) return audio_sources;
    if constexpr (std::is_same_v<T, audio_listener_component>) return audio_listeners;

    static std::unordered_map<entity, T> dummy;
    return dummy;
}

template<typename T>
inline const std::unordered_map<entity, T>& registry::pool() const
{
    if constexpr (std::is_same_v<T, transform_component>) return transforms;
    if constexpr (std::is_same_v<T, mesh_component>) return meshes;
    if constexpr (std::is_same_v<T, camera_component>) return cameras;
    if constexpr (std::is_same_v<T, light_component>) return lights;
    if constexpr (std::is_same_v<T, name_component>) return names;
    if constexpr (std::is_same_v<T, bounds_component>) return bounds;
    if constexpr (std::is_same_v<T, material_component>) return materials;
    if constexpr (std::is_same_v<T, rigidbody_component>) return rigidbodies;
    if constexpr (std::is_same_v<T, collider_component>) return colliders;
    if constexpr (std::is_same_v<T, native_script_component>) return scriptable_components;
    if constexpr (std::is_same_v<T, hierarchy_component>) return hierarchies;
    if constexpr (std::is_same_v<T, skinned_mesh_component>) return skinned_meshes;
    if constexpr (std::is_same_v<T, skeleton_component>) return skeletons;
    if constexpr (std::is_same_v<T, animator_component>) return animators;
    if constexpr (std::is_same_v<T, bone_attachment_component>) return bone_attachments;
    if constexpr (std::is_same_v<T, canvas_component>) return canvases;
    if constexpr (std::is_same_v<T, rect_transform_component>) return rect_transforms;
    if constexpr (std::is_same_v<T, ui_text_component>) return ui_texts;
    if constexpr (std::is_same_v<T, ui_image_component>) return ui_images;
    if constexpr (std::is_same_v<T, ui_button_component>) return ui_buttons;
    if constexpr (std::is_same_v<T, audio_source_component>) return audio_sources;
    if constexpr (std::is_same_v<T, audio_listener_component>) return audio_listeners;

    static std::unordered_map<entity, T> dummy;
    return dummy;
}