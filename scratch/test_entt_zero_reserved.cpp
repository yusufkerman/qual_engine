#include <iostream>
#include <entt/entt.hpp>

int main() {
    entt::registry reg;
    // Occupy 0 so it's never handed out to users
    auto reserved_zero = reg.create(entt::entity{0});
    std::cout << "Reserved: " << entt::to_integral(reserved_zero) << "\n";

    auto e1 = reg.create();
    auto e2 = reg.create();
    std::cout << "e1: " << entt::to_integral(e1) << "\n";
    std::cout << "e2: " << entt::to_integral(e2) << "\n";

    reg.destroy(e1);
    auto e3 = reg.create();
    std::cout << "e3: " << entt::to_integral(e3) << "\n";

    return 0;
}

