#include <iostream>
#include <vector>
#include <entt/entt.hpp>

using entity = uint32_t;

inline entt::entity to_entt(entity e) noexcept {
    return static_cast<entt::entity>(e);
}

inline entity to_entity(entt::entity e) noexcept {
    return static_cast<entity>(e);
}

template<typename T>
class entt_component_pool {
public:
    struct value_proxy {
        entity first;
        T& second;
        value_proxy(entity e, T& ref) : first(e), second(ref) {}
    };

    struct const_value_proxy {
        entity first;
        const T& second;
        const_value_proxy(entity e, const T& ref) : first(e), second(ref) {}
    };

    class iterator {
    public:
        using storage_type = typename entt::storage_for_t<T>;
        
        iterator(storage_type* storage, size_t index) 
            : m_storage(storage), m_index(index) {}

        bool operator!=(const iterator& other) const {
            return m_index != other.m_index;
        }

        bool operator==(const iterator& other) const {
            return m_index == other.m_index;
        }

        iterator& operator++() {
            ++m_index;
            return *this;
        }

        value_proxy operator*() const {
            auto ent = m_storage->data()[m_index];
            return value_proxy{ to_entity(ent), m_storage->get(ent) };
        }

    private:
        storage_type* m_storage;
        size_t m_index;
    };

    class const_iterator {
    public:
        using storage_type = const typename entt::storage_for_t<T>;
        
        const_iterator(storage_type* storage, size_t index) 
            : m_storage(storage), m_index(index) {}

        bool operator!=(const const_iterator& other) const {
            return m_index != other.m_index;
        }

        bool operator==(const const_iterator& other) const {
            return m_index == other.m_index;
        }

        const_iterator& operator++() {
            ++m_index;
            return *this;
        }

        const_value_proxy operator*() const {
            auto ent = m_storage->data()[m_index];
            return const_value_proxy{ to_entity(ent), m_storage->get(ent) };
        }

    private:
        storage_type* m_storage;
        size_t m_index;
    };

    explicit entt_component_pool(entt::registry* reg) : m_reg(reg) {}

    size_t size() const noexcept {
        return m_reg ? m_reg->storage<T>().size() : 0;
    }

    bool empty() const noexcept {
        return size() == 0;
    }

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

private:
    entt::registry* m_reg = nullptr;
};

struct position {
    float x = 0.0f;
    float y = 0.0f;
};

int main() {
    entt::registry raw_reg;
    entt_component_pool<position> pool(&raw_reg);

    auto e1 = raw_reg.create();
    auto e2 = raw_reg.create();
    raw_reg.emplace<position>(e1, 10.0f, 20.0f);
    raw_reg.emplace<position>(e2, 30.0f, 40.0f);

    std::cout << "Pool size: " << pool.size() << "\n";

    // What if loop is auto [e, pos]?
    for (auto [e, pos] : pool) {
        std::cout << "auto [e, pos] : entity " << e << " -> (" << pos.x << ", " << pos.y << ")\n";
    }

    return 0;
}

