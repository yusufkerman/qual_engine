#include <iostream>
#include <entt/entt.hpp>

struct position { float x, y; };

int main() {
    entt::registry reg;
    auto e = reg.create();
    // remove component when not present
    auto count = reg.remove<position>(e);
    std::cout << "Removed count: " << count << "\n";
    return 0;
}

