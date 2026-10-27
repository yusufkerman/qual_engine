#include <iostream>
#include <tuple>
#include <utility>

struct my_tuple_ref {
    uint32_t ent;
    int* val_ptr;

    template<size_t I>
    decltype(auto) get() const {
        if constexpr (I == 0) return (ent);
        else if constexpr (I == 1) return (*val_ptr);
    }
};

namespace std {
    template<>
    struct tuple_size<my_tuple_ref> : std::integral_constant<size_t, 2> {};

    template<size_t I>
    struct tuple_element<I, my_tuple_ref> {
        using type = std::conditional_t<I == 0, uint32_t, int&>;
    };
}

struct my_iter {
    uint32_t id;
    int* val;

    my_tuple_ref operator*() const {
        return my_tuple_ref{ id, val };
    }
    my_iter& operator++() { ++id; return *this; }
    bool operator!=(const my_iter& o) const { return id != o.id; }
};

struct my_range {
    int x = 42;
    my_iter begin() { return my_iter{ 1, &x }; }
    my_iter end() { return my_iter{ 2, &x }; }
};

int main() {
    my_range r;
    // Test auto& [e, comp] on temporary my_tuple_ref:
    for (auto& [e, comp] : r) {
        std::cout << e << " = " << comp << "\n";
        comp = 100;
    }
    std::cout << "After mod: " << r.x << "\n";
    return 0;
}

