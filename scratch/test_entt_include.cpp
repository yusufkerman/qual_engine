#include <entt/entt.hpp>
#include <iostream>

int main() {
    entt::registry reg;
    auto e = reg.create();
    reg.emplace<int>(e, 42);
    auto val = reg.get<int>(e);
    std::cout << "EnTT compiled successfully! Value: " << val << "\n";
    return 0;
}

