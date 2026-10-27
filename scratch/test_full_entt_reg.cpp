#include <iostream>
#include <unordered_set>
#include <tuple>
#include <entt/entt.hpp>

using entity = uint32_t;

inline entt::entity to_entt(entity e) noexcept {
    return static_cast<entt::entity>(e);
}

inline entity to_entity(entt::entity e) noexcept {
    return static_cast<entity>(e);
}

// Forward declare registry
class test_reg;

template<typename T>
struct comp_pair {
    entity first;
    T& second;
};

template<typename T>
struct const_comp_pair {
    entity first;
    const T& second;
};

template<typename T>
class entt_component_pool {
public:
    class iterator {
    public:
        using storage_type = typename entt::storage_for_t<T>;

        iterator(storage_type* s, size_t idx) : m_s(s), m_idx(idx) {}

        ~iterator() {
            if (m_constructed) {
                std::destroy_at(ptr());
            }
        }

        iterator(const iterator& o) : m_s(o.m_s), m_idx(o.m_idx), m_constructed(false) {}

        iterator& operator=(const iterator& o) {
            if (this != &o) {
                if (m_constructed) {
                    std::destroy_at(ptr());
                    m_constructed = false;
                }
                m_s = o.m_s;
                m_idx = o.m_idx;
            }
            return *this;
        }

        bool operator!=(const iterator& o) const { return m_idx != o.m_idx; }
        bool operator==(const iterator& o) const { return m_idx == o.m_idx; }

        iterator& operator++() {
            if (m_constructed) {
                std::destroy_at(ptr());
                m_constructed = false;
            }
            ++m_idx;
            return *this;
        }

        comp_pair<T>& operator*() const {
            if (m_constructed) {
                std::destroy_at(ptr());
                m_constructed = false;
            }
            auto ent = m_s->data()[m_idx];
            T& comp = m_s->get(ent);
            std::construct_at(ptr(), to_entity(ent), comp);
            m_constructed = true;
            return *ptr();
        }

    private:
        comp_pair<T>* ptr() const {
            return reinterpret_cast<comp_pair<T>*>(m_buf);
        }

        storage_type* m_s;
        size_t m_idx;
        mutable bool m_constructed = false;
        alignas(comp_pair<T>) mutable uint8_t m_buf[sizeof(comp_pair<T>)];
    };

    class const_iterator {
    public:
        using storage_type = const typename entt::storage_for_t<T>;

        const_iterator(storage_type* s, size_t idx) : m_s(s), m_idx(idx) {}

        ~const_iterator() {
            if (m_constructed) {
                std::destroy_at(ptr());
            }
        }

        const_iterator(const const_iterator& o) : m_s(o.m_s), m_idx(o.m_idx), m_constructed(false) {}

        const_iterator& operator=(const const_iterator& o) {
            if (this != &o) {
                if (m_constructed) {
                    std::destroy_at(ptr());
                    m_constructed = false;
                }
                m_s = o.m_s;
                m_idx = o.m_idx;
            }
            return *this;
        }

        bool operator!=(const const_iterator& o) const { return m_idx != o.m_idx; }
        bool operator==(const const_iterator& o) const { return m_idx == o.m_idx; }

        const_iterator& operator++() {
            if (m_constructed) {
                std::destroy_at(ptr());
                m_constructed = false;
            }
            ++m_idx;
            return *this;
        }

        const_comp_pair<T>& operator*() const {
            if (m_constructed) {
                std::destroy_at(ptr());
                m_constructed = false;
            }
            auto ent = m_s->data()[m_idx];
            const T& comp = m_s->get(ent);
            std::construct_at(ptr(), to_entity(ent), comp);
            m_constructed = true;
            return *ptr();
        }

    private:
        const_comp_pair<T>* ptr() const {
            return reinterpret_cast<const_comp_pair<T>*>(m_buf);
        }

        storage_type* m_s;
        size_t m_idx;
        mutable bool m_constructed = false;
        alignas(const_comp_pair<T>) mutable uint8_t m_buf[sizeof(const_comp_pair<T>)];
    };

    explicit entt_component_pool(entt::registry* r) : m_reg(r) {}

    void set_registry(entt::registry* r) noexcept { m_reg = r; }

    iterator begin() {
        auto& s = m_reg->storage<T>();
        return iterator(&s, 0);
    }

    iterator end() {
        auto& s = m_reg->storage<T>();
        return iterator(&s, s.size());
    }

    const_iterator begin() const {
        const auto& s = m_reg->storage<T>();
        return const_iterator(&s, 0);
    }

    const_iterator end() const {
        const auto& s = m_reg->storage<T>();
        return const_iterator(&s, s.size());
    }

    size_t size() const noexcept {
        return m_reg ? m_reg->storage<T>().size() : 0;
    }

    bool empty() const noexcept {
        return size() == 0;
    }

private:
    entt::registry* m_reg = nullptr;
};

struct pos_comp { float x = 0; float y = 0; };
struct tag_comp { std::string tag; };

class test_reg {
public:
    test_reg() {
        init();
    }

    ~test_reg() = default;

    test_reg(test_reg&& other) noexcept 
        : m_entt(std::move(other.m_entt)),
          m_entities(std::move(other.m_entities))
    {
        rebind_pools();
    }

    test_reg& operator=(test_reg&& other) noexcept {
        if (this != &other) {
            m_entt = std::move(other.m_entt);
            m_entities = std::move(other.m_entities);
            rebind_pools();
        }
        return *this;
    }

    test_reg(const test_reg& other) {
        init();
        copy_from(other);
    }

    test_reg& operator=(const test_reg& other) {
        if (this != &other) {
            clear();
            copy_from(other);
        }
        return *this;
    }

    entity create_entity() {
        auto ent = m_entt.create();
        entity e = to_entity(ent);
        m_entities.insert(e);
        return e;
    }

    void add_entity(entity e) {
        if (e == 0) return;
        auto ent = to_entt(e);
        if (!m_entt.valid(ent)) {
            m_entt.create(ent);
        }
        m_entities.insert(e);
    }

    void remove_entity(entity e) {
        if (e == 0) return;
        auto ent = to_entt(e);
        if (m_entt.valid(ent)) {
            m_entt.destroy(ent);
        }
        m_entities.erase(e);
    }

    const std::unordered_set<entity>& get_all_entities() const {
        return m_entities;
    }

    template<typename T, typename... Args>
    T& add_component(entity e, Args&&... args) {
        return m_entt.emplace_or_replace<T>(to_entt(e), std::forward<Args>(args)...);
    }

    template<typename T>
    T* get_component(entity e) {
        if (e == 0) return nullptr;
        return m_entt.try_get<T>(to_entt(e));
    }

    template<typename T>
    const T* get_component(entity e) const {
        if (e == 0) return nullptr;
        return m_entt.try_get<T>(to_entt(e));
    }

    template<typename T>
    bool has_component(entity e) const {
        if (e == 0) return false;
        return m_entt.all_of<T>(to_entt(e));
    }

    template<typename T>
    void remove_component(entity e) {
        if (e == 0) return;
        m_entt.remove<T>(to_entt(e));
    }

    template<typename T>
    entt_component_pool<T>& get_all() {
        return pool<T>();
    }

    template<typename T>
    const entt_component_pool<T>& get_all() const {
        return pool<T>();
    }

    void clone_entity(entity e, test_reg& dst) const {
        dst.add_entity(e);
        copy_entity_components(e, e, dst);
    }

    void clear() {
        m_entt.clear();
        m_entities.clear();
        m_entt.create(to_entt(0)); // Reserve 0
    }

private:
    void init() {
        m_entt.create(to_entt(0)); // Reserve 0
        rebind_pools();
    }

    void rebind_pools() {
        m_pool_pos.set_registry(&m_entt);
        m_pool_tag.set_registry(&m_entt);
    }

    void copy_from(const test_reg& other) {
        for (entity e : other.m_entities) {
            other.clone_entity(e, *this);
        }
    }

    void copy_entity_components(entity src, entity dst, test_reg& dst_reg) const {
        if (const auto* c = get_component<pos_comp>(src)) dst_reg.add_component<pos_comp>(dst, *c);
        if (const auto* c = get_component<tag_comp>(src)) dst_reg.add_component<tag_comp>(dst, *c);
    }

    template<typename T>
    entt_component_pool<T>& pool() {
        if constexpr (std::is_same_v<T, pos_comp>) return m_pool_pos;
        if constexpr (std::is_same_v<T, tag_comp>) return m_pool_tag;
    }

    template<typename T>
    const entt_component_pool<T>& pool() const {
        if constexpr (std::is_same_v<T, pos_comp>) return m_pool_pos;
        if constexpr (std::is_same_v<T, tag_comp>) return m_pool_tag;
    }

    entt::registry m_entt;
    std::unordered_set<entity> m_entities;

    entt_component_pool<pos_comp> m_pool_pos{ nullptr };
    entt_component_pool<tag_comp> m_pool_tag{ nullptr };
};

int main() {
    test_reg r1;
    auto e1 = r1.create_entity();
    auto e2 = r1.create_entity();
    r1.add_component<pos_comp>(e1, 10.0f, 20.0f);
    r1.add_component<tag_comp>(e1, "Player");
    r1.add_component<pos_comp>(e2, 50.0f, 60.0f);

    std::cout << "r1 pos count: " << r1.get_all<pos_comp>().size() << "\n";
    for (auto& [e, p] : r1.get_all<pos_comp>()) {
        std::cout << "r1 entity " << e << ": (" << p.x << ", " << p.y << ")\n";
        p.x += 5.0f;
    }

    // Copy constructor test
    test_reg r2 = r1;
    std::cout << "r2 copied. pos count: " << r2.get_all<pos_comp>().size() << "\n";
    for (const auto& [e, p] : r2.get_all<pos_comp>()) {
        std::cout << "r2 entity " << e << ": (" << p.x << ", " << p.y << ")\n";
    }

    // Move assignment test
    test_reg r3;
    r3 = std::move(r2);
    std::cout << "r3 moved. pos count: " << r3.get_all<pos_comp>().size() << "\n";
    for (auto& [e, p] : r3.get_all<pos_comp>()) {
        std::cout << "r3 entity " << e << ": (" << p.x << ", " << p.y << ")\n";
    }

    // Removal test
    r1.remove_entity(e1);
    std::cout << "After removing e1, r1 pos count: " << r1.get_all<pos_comp>().size() << "\n";
    std::cout << "r1 e1 has pos: " << r1.has_component<pos_comp>(e1) << "\n";
    std::cout << "r1 e2 has pos: " << r1.has_component<pos_comp>(e2) << "\n";

    return 0;
}

