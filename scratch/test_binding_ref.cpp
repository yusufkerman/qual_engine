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
    ent_comp_pair<T> operator*() const {
        return ent_comp_pair<T>{ id, *val };
    }
    fake_iterator& operator++() { ++id; return *this; }
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
    // Test if for (auto& [e, comp] : v) compiles:
    for (auto& [e, comp] : v) {
        std::cout << e << ": " << comp << "\n";
    }
    return 0;
}

