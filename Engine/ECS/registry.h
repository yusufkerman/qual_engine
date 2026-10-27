#pragma once

/**
 * @file registry.h
 * @brief EnTT v3.13.2 tabanlı, sektör standardı Sparse-Set ECS kayıt defteri.
 *
 * QualEngine için yüksek performanslı, önbellek dostu (cache-friendly) ve
 * tip-güvenli varlık ve bileşen yönetim katmanıdır.
 *
 * Mimari Özellikler:
 * - Sparse-Set depolama: O(1) bileşen ekleme, silme ve arama.
 * - Bellek yerelliği: Bitişik bellek dizilerinde (contiguous arrays) bileşen iterasyonu.
 * - Tam geriye dönük uyumluluk: Mevcut motor sistemleri ve Vesta panelleri hiçbir
 *   kod değişikliği gerektirmeden çalışır.
 * - Modern EnTT API erişimi: reg.view<T1, T2>(), reg.raw() ve nesilsel (generational)
 *   ID güvenliği.
 */

#include <entt/entt.hpp>
#include <unordered_set>
#include <tuple>
#include <utility>
#include <type_traits>
#include <memory>
#include "entity.h"
#include "components.h"

// ============================================================================
// ENTITY CONVERSION HELPERS
// ============================================================================

/**
 * @brief QualEngine entity tipini (uint32_t) EnTT entt::entity tipine dönüştürür.
 */
[[nodiscard]] inline constexpr entt::entity to_entt(entity e) noexcept
{
    return static_cast<entt::entity>(e);
}

/**
 * @brief EnTT entt::entity tipini QualEngine entity (uint32_t) tipine dönüştürür.
 */
[[nodiscard]] inline constexpr entity to_entity(entt::entity e) noexcept
{
    return static_cast<entity>(e);
}

// ============================================================================
// ENTT COMPONENT POOL ADAPTER
// ============================================================================

/**
 * @class entt_component_pool
 * @brief EnTT Sparse-Set bileşen havuzunu QualEngine API'siyle köprüleyen adaptör.
 *
 * Range-for döngülerinde structured binding [e, comp] ve in-place bileşen
 * manipülasyonunu destekler.
 */
template<typename T>
class entt_component_pool
{
public:
    struct value_proxy
    {
        entity first;
        T& second;
        value_proxy(entity e, T& ref) : first(e), second(ref) {}
    };

    struct const_value_proxy
    {
        entity first;
        const T& second;
        const_value_proxy(entity e, const T& ref) : first(e), second(ref) {}
    };

    class iterator
    {
    public:
        using storage_type = typename entt::storage_for_t<T>;
        using iterator_category = std::forward_iterator_tag;
        using value_type = value_proxy;
        using difference_type = std::ptrdiff_t;
        using pointer = value_proxy*;
        using reference = value_proxy&;

        iterator(storage_type* s, size_t idx) : m_s(s), m_idx(idx) {}

        ~iterator()
        {
            destroy_current();
        }

        iterator(const iterator& o) : m_s(o.m_s), m_idx(o.m_idx), m_constructed(false) {}

        iterator& operator=(const iterator& o)
        {
            if (this != &o)
            {
                destroy_current();
                m_s = o.m_s;
                m_idx = o.m_idx;
            }
            return *this;
        }

        bool operator!=(const iterator& o) const noexcept { return m_idx != o.m_idx; }
        bool operator==(const iterator& o) const noexcept { return m_idx == o.m_idx; }

        iterator& operator++()
        {
            destroy_current();
            ++m_idx;
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        value_proxy& operator*() const
        {
            destroy_current();
            const auto ent = m_s->data()[m_idx];
            T& comp = m_s->get(ent);
            std::construct_at(ptr(), to_entity(ent), comp);
            m_constructed = true;
            return *ptr();
        }

        value_proxy* operator->() const
        {
            return &(**this);
        }

    private:
        value_proxy* ptr() const noexcept
        {
            return reinterpret_cast<value_proxy*>(m_buf);
        }

        void destroy_current() const noexcept
        {
            if (m_constructed)
            {
                std::destroy_at(ptr());
                m_constructed = false;
            }
        }

        storage_type* m_s = nullptr;
        size_t m_idx = 0;
        mutable bool m_constructed = false;
        alignas(value_proxy) mutable uint8_t m_buf[sizeof(value_proxy)];
    };

    class const_iterator
    {
    public:
        using storage_type = const typename entt::storage_for_t<T>;
        using iterator_category = std::forward_iterator_tag;
        using value_type = const_value_proxy;
        using difference_type = std::ptrdiff_t;
        using pointer = const const_value_proxy*;
        using reference = const const_value_proxy&;

        const_iterator(storage_type* s, size_t idx) : m_s(s), m_idx(idx) {}

        ~const_iterator()
        {
            destroy_current();
        }

        const_iterator(const const_iterator& o) : m_s(o.m_s), m_idx(o.m_idx), m_constructed(false) {}

        const_iterator& operator=(const const_iterator& o)
        {
            if (this != &o)
            {
                destroy_current();
                m_s = o.m_s;
                m_idx = o.m_idx;
            }
            return *this;
        }

        bool operator!=(const const_iterator& o) const noexcept { return m_idx != o.m_idx; }
        bool operator==(const const_iterator& o) const noexcept { return m_idx == o.m_idx; }

        const_iterator& operator++()
        {
            destroy_current();
            ++m_idx;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const_value_proxy& operator*() const
        {
            destroy_current();
            const auto ent = m_s->data()[m_idx];
            const T& comp = m_s->get(ent);
            std::construct_at(ptr(), to_entity(ent), comp);
            m_constructed = true;
            return *ptr();
        }

        const const_value_proxy* operator->() const
        {
            return &(**this);
        }

    private:
        const_value_proxy* ptr() const noexcept
        {
            return reinterpret_cast<const_value_proxy*>(m_buf);
        }

        void destroy_current() const noexcept
        {
            if (m_constructed)
            {
                std::destroy_at(ptr());
                m_constructed = false;
            }
        }

        storage_type* m_s = nullptr;
        size_t m_idx = 0;
        mutable bool m_constructed = false;
        alignas(const_value_proxy) mutable uint8_t m_buf[sizeof(const_value_proxy)];
    };

    explicit entt_component_pool(entt::registry* r) noexcept : m_reg(r) {}

    void set_registry(entt::registry* r) noexcept { m_reg = r; }

    [[nodiscard]] size_t size() const noexcept
    {
        return m_reg ? m_reg->storage<T>().size() : 0;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return size() == 0;
    }

    [[nodiscard]] bool contains(entity e) const noexcept
    {
        if (e == 0 || !m_reg) return false;
        return m_reg->storage<T>().contains(to_entt(e));
    }

    iterator begin()
    {
        auto& s = m_reg->storage<T>();
        return iterator(&s, 0);
    }

    iterator end()
    {
        auto& s = m_reg->storage<T>();
        return iterator(&s, s.size());
    }

    const_iterator begin() const
    {
        const auto& s = m_reg->storage<T>();
        return const_iterator(&s, 0);
    }

    const_iterator end() const
    {
        const auto& s = m_reg->storage<T>();
        return const_iterator(&s, s.size());
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    const_iterator cend() const
    {
        return end();
    }

    iterator find(entity e)
    {
        if (e == 0 || !m_reg) return end();
        auto ent = to_entt(e);
        auto& s = m_reg->storage<T>();
        if (!s.contains(ent)) return end();
        return iterator(&s, s.index(ent));
    }

    const_iterator find(entity e) const
    {
        if (e == 0 || !m_reg) return end();
        auto ent = to_entt(e);
        const auto& s = m_reg->storage<T>();
        if (!s.contains(ent)) return end();
        return const_iterator(&s, s.index(ent));
    }

    [[nodiscard]] typename entt::storage_for_t<T>& raw() noexcept
    {
        return m_reg->storage<T>();
    }

    [[nodiscard]] const typename entt::storage_for_t<T>& raw() const noexcept
    {
        return m_reg->storage<T>();
    }

private:
    entt::registry* m_reg = nullptr;
};

// ============================================================================
// ECS REGISTRY CLASS
// ============================================================================

class registry
{
public:
    registry();
    ~registry() = default;

    registry(const registry& other);
    registry& operator=(const registry& other);
    registry(registry&& other) noexcept;
    registry& operator=(registry&& other) noexcept;

    /**
     * @brief Yeni ve benzersiz bir varlık (Entity) oluşturur.
     */
    entity create_entity();

    /**
     * @brief Belirli bir varlık kimliğini sisteme kaydeder (Serileştirme/Snapshot).
     */
    void add_entity(entity e);

    /**
     * @brief Varlığı ve ona bağlı olan tüm bileşenleri bellekten siler.
     */
    void remove_entity(entity e);

    /**
     * @brief Kayıtlı tüm varlıkların kümesini döner.
     */
    [[nodiscard]] const std::unordered_set<entity>& get_all_entities() const noexcept;

    /**
     * @brief Varlığın kayıt defterinde geçerli (canlı) olup olmadığını denetler.
     */
    [[nodiscard]] bool valid(entity e) const noexcept;

    /**
     * @brief Kayıt defterindeki tüm varlık ve bileşen havuzlarını sıfırlar.
     */
    void clear();

    // =========================
    // COMPONENT API
    // =========================

    template<typename T, typename... Args>
    T& add_component(entity e, Args&&... args)
    {
        return m_entt.emplace_or_replace<T>(to_entt(e), std::forward<Args>(args)...);
    }

    template<typename T>
    T* get_component(entity e)
    {
        if (e == 0) return nullptr;
        return m_entt.try_get<T>(to_entt(e));
    }

    template<typename T>
    const T* get_component(entity e) const
    {
        if (e == 0) return nullptr;
        return m_entt.try_get<T>(to_entt(e));
    }

    template<typename T>
    bool has_component(entity e) const
    {
        if (e == 0) return false;
        return m_entt.all_of<T>(to_entt(e));
    }

    template<typename T>
    void remove_component(entity e)
    {
        if (e == 0) return;
        m_entt.remove<T>(to_entt(e));
    }

    template<typename T>
    entt_component_pool<T>& get_all()
    {
        return pool<T>();
    }

    template<typename T>
    const entt_component_pool<T>& get_all() const
    {
        return pool<T>();
    }

    // =========================
    // MODERN ENTT VIEW API
    // =========================

    template<typename... Components>
    [[nodiscard]] auto view()
    {
        return m_entt.view<Components...>();
    }

    template<typename... Components>
    [[nodiscard]] auto view() const
    {
        return m_entt.view<Components...>();
    }

    [[nodiscard]] entt::registry& raw() noexcept
    {
        return m_entt;
    }

    [[nodiscard]] const entt::registry& raw() const noexcept
    {
        return m_entt;
    }

    // =========================
    // CLONE SYSTEM
    // =========================

    /**
     * @brief Belirtilen varlığı aynı kimlikle hedef kayıt defterine kopyalar.
     */
    void clone_entity(entity e, registry& dst) const;

    /**
     * @brief Belirtilen varlığın yeni bir kimlikle tam bir kopyasını oluşturur.
     */
    entity duplicate_entity(entity source);

private:
    void init();
    void rebind_pools();
    void copy_components_internal(entity srcEntity, entity dstEntity, registry& dstRegistry) const;

    entt::registry m_entt;
    std::unordered_set<entity> m_entities;

    // Motorun 22 Standart Bileşen Havuz Adaptörü
    entt_component_pool<transform_component> m_pool_transforms{ nullptr };
    entt_component_pool<mesh_component> m_pool_meshes{ nullptr };
    entt_component_pool<camera_component> m_pool_cameras{ nullptr };
    entt_component_pool<light_component> m_pool_lights{ nullptr };
    entt_component_pool<name_component> m_pool_names{ nullptr };
    entt_component_pool<bounds_component> m_pool_bounds{ nullptr };
    entt_component_pool<material_component> m_pool_materials{ nullptr };
    entt_component_pool<rigidbody_component> m_pool_rigidbodies{ nullptr };
    entt_component_pool<collider_component> m_pool_colliders{ nullptr };
    entt_component_pool<native_script_component> m_pool_scriptables{ nullptr };
    entt_component_pool<hierarchy_component> m_pool_hierarchies{ nullptr };
    entt_component_pool<skinned_mesh_component> m_pool_skinned_meshes{ nullptr };
    entt_component_pool<skeleton_component> m_pool_skeletons{ nullptr };
    entt_component_pool<animator_component> m_pool_animators{ nullptr };
    entt_component_pool<bone_attachment_component> m_pool_bone_attachments{ nullptr };
    entt_component_pool<canvas_component> m_pool_canvases{ nullptr };
    entt_component_pool<rect_transform_component> m_pool_rect_transforms{ nullptr };
    entt_component_pool<ui_text_component> m_pool_ui_texts{ nullptr };
    entt_component_pool<ui_image_component> m_pool_ui_images{ nullptr };
    entt_component_pool<ui_button_component> m_pool_ui_buttons{ nullptr };
    entt_component_pool<audio_source_component> m_pool_audio_sources{ nullptr };
    entt_component_pool<audio_listener_component> m_pool_audio_listeners{ nullptr };

    template<typename T>
    entt_component_pool<T>& pool();

    template<typename T>
    const entt_component_pool<T>& pool() const;
};

// ============================================================================
// POOL ACCESSORS (INLINE)
// ============================================================================

template<typename T>
inline entt_component_pool<T>& registry::pool()
{
    if constexpr (std::is_same_v<T, transform_component>) return m_pool_transforms;
    if constexpr (std::is_same_v<T, mesh_component>) return m_pool_meshes;
    if constexpr (std::is_same_v<T, camera_component>) return m_pool_cameras;
    if constexpr (std::is_same_v<T, light_component>) return m_pool_lights;
    if constexpr (std::is_same_v<T, name_component>) return m_pool_names;
    if constexpr (std::is_same_v<T, bounds_component>) return m_pool_bounds;
    if constexpr (std::is_same_v<T, material_component>) return m_pool_materials;
    if constexpr (std::is_same_v<T, rigidbody_component>) return m_pool_rigidbodies;
    if constexpr (std::is_same_v<T, collider_component>) return m_pool_colliders;
    if constexpr (std::is_same_v<T, native_script_component>) return m_pool_scriptables;
    if constexpr (std::is_same_v<T, hierarchy_component>) return m_pool_hierarchies;
    if constexpr (std::is_same_v<T, skinned_mesh_component>) return m_pool_skinned_meshes;
    if constexpr (std::is_same_v<T, skeleton_component>) return m_pool_skeletons;
    if constexpr (std::is_same_v<T, animator_component>) return m_pool_animators;
    if constexpr (std::is_same_v<T, bone_attachment_component>) return m_pool_bone_attachments;
    if constexpr (std::is_same_v<T, canvas_component>) return m_pool_canvases;
    if constexpr (std::is_same_v<T, rect_transform_component>) return m_pool_rect_transforms;
    if constexpr (std::is_same_v<T, ui_text_component>) return m_pool_ui_texts;
    if constexpr (std::is_same_v<T, ui_image_component>) return m_pool_ui_images;
    if constexpr (std::is_same_v<T, ui_button_component>) return m_pool_ui_buttons;
    if constexpr (std::is_same_v<T, audio_source_component>) return m_pool_audio_sources;
    if constexpr (std::is_same_v<T, audio_listener_component>) return m_pool_audio_listeners;

    static entt_component_pool<T> fallback{ nullptr };
    fallback.set_registry(&m_entt);
    return fallback;
}

template<typename T>
inline const entt_component_pool<T>& registry::pool() const
{
    if constexpr (std::is_same_v<T, transform_component>) return m_pool_transforms;
    if constexpr (std::is_same_v<T, mesh_component>) return m_pool_meshes;
    if constexpr (std::is_same_v<T, camera_component>) return m_pool_cameras;
    if constexpr (std::is_same_v<T, light_component>) return m_pool_lights;
    if constexpr (std::is_same_v<T, name_component>) return m_pool_names;
    if constexpr (std::is_same_v<T, bounds_component>) return m_pool_bounds;
    if constexpr (std::is_same_v<T, material_component>) return m_pool_materials;
    if constexpr (std::is_same_v<T, rigidbody_component>) return m_pool_rigidbodies;
    if constexpr (std::is_same_v<T, collider_component>) return m_pool_colliders;
    if constexpr (std::is_same_v<T, native_script_component>) return m_pool_scriptables;
    if constexpr (std::is_same_v<T, hierarchy_component>) return m_pool_hierarchies;
    if constexpr (std::is_same_v<T, skinned_mesh_component>) return m_pool_skinned_meshes;
    if constexpr (std::is_same_v<T, skeleton_component>) return m_pool_skeletons;
    if constexpr (std::is_same_v<T, animator_component>) return m_pool_animators;
    if constexpr (std::is_same_v<T, bone_attachment_component>) return m_pool_bone_attachments;
    if constexpr (std::is_same_v<T, canvas_component>) return m_pool_canvases;
    if constexpr (std::is_same_v<T, rect_transform_component>) return m_pool_rect_transforms;
    if constexpr (std::is_same_v<T, ui_text_component>) return m_pool_ui_texts;
    if constexpr (std::is_same_v<T, ui_image_component>) return m_pool_ui_images;
    if constexpr (std::is_same_v<T, ui_button_component>) return m_pool_ui_buttons;
    if constexpr (std::is_same_v<T, audio_source_component>) return m_pool_audio_sources;
    if constexpr (std::is_same_v<T, audio_listener_component>) return m_pool_audio_listeners;

    static entt_component_pool<T> fallback{ nullptr };
    fallback.set_registry(const_cast<entt::registry*>(&m_entt));
    return fallback;
}
