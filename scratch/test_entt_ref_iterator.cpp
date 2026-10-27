#include <iostream>
#include <memory>
#include <entt/entt.hpp>

using entity = uint32_t;

template<typename T>
struct comp_pair {
    entity first;
    T& second;
};

template<typename T>
class my_entt_pool {
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
            std::construct_at(ptr(), static_cast<entity>(ent), comp);
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

    explicit my_entt_pool(entt::registry* r) : m_reg(r) {}

    iterator begin() {
        auto& s = m_reg->storage<T>();
        return iterator(&s, 0);
    }

    iterator end() {
        auto& s = m_reg->storage<T>();
        return iterator(&s, s.size());
    }

    size_t size() const {
        return m_reg->storage<T>().size();
    }

private:
    entt::registry* m_reg;
};

struct position {
    float x = 0.0f;
    float y = 0.0f;
};

int main() {
    entt::registry reg;
    auto e1 = reg.create();
    auto e2 = reg.create();
    reg.emplace<position>(e1, 10.0f, 20.0f);
    reg.emplace<position>(e2, 30.0f, 40.0f);

    my_entt_pool<position> pool(&reg);

    // Test 1: auto& [e, comp]
    for (auto& [e, comp] : pool) {
        std::cout << "auto& entity " << e << " = (" << comp.x << ", " << comp.y << ")\n";
        comp.x += 100.0f;
    }

    // Test 2: const auto& [e, comp]
    for (const auto& [e, comp] : pool) {
        std::cout << "const auto& entity " << e << " = (" << comp.x << ", " << comp.y << ")\n";
    }

    // Test 3: auto [e, comp]
    for (auto [e, comp] : pool) {
        std::cout << "auto copy entity " << e << " = (" << comp.x << ", " << comp.y << ")\n";
    }

    // Test 4: auto& ref = pool
    auto& pool_ref = pool;
    std::cout << "pool_ref.size(): " << pool_ref.size() << "\n";
    for (auto& [e, comp] : pool_ref) {
        std::cout << "ref loop entity " << e << " = (" << comp.x << ", " << comp.y << ")\n";
    }

    return 0;
}

