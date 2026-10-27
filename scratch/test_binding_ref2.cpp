#include <iostream>
#include <vector>

template<typename T>
struct ent_comp_pair {
    uint32_t first;
    T& second;
};

template<typename T>
struct fake_iterator {
    uint32_t id;
    T* val;
    mutable ent_comp_pair<T> current{ id, *val };

    fake_iterator(uint32_t i, T* v) : id(i), val(v), current{ i, *v } {}

    ent_comp_pair<T>& operator*() const {
        current.first = id;
        current.second = *val;
        return current;
    }
    fake_iterator& operator++() {
        ++id;
        return *this;
    }
    bool operator!=(const fake_iterator& o) const { return id != o.id; }
};

template<typename T>
struct fake_view {
    T data = 42;
    fake_iterator<T> begin() { return fake_iterator<T>{ 1, &data }; }
    fake_iterator<T> end() { return fake_iterator<T>{ 2, &data }; }
};

int main() {
    fake_view<int> v;
    for (auto& [e, comp] : v) {
        std::cout << e << ": " << comp << "\n";
        comp = 100;
    }
    for (const auto& [e, comp] : v) {
        std::cout << e << ": " << comp << "\n";
    }
    return 0;
}

