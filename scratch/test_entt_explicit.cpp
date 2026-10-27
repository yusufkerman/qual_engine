#include <iostream>
#include <entt/entt.hpp>

int main() {
    entt::registry reg;
    // Create entity with explicit ID 5
    auto e5 = reg.create(entt::entity{5});
    std::cout << "e5: " << entt::to_integral(e5) << ", valid: " << reg.valid(e5) << "\n";

    // Now call reg.create()
    auto e_auto1 = reg.create();
    auto e_auto2 = reg.create();
    std::cout << "e_auto1: " << entt::to_integral(e_auto1) << "\n";
    std::cout << "e_auto2: " << entt::to_integral(e_auto2) << "\n";

    // Create entity with explicit ID 1
    auto e1_check = reg.valid(entt::entity{1});
    std::cout << "is 1 valid before explicit: " << e1_check << "\n";

    return 0;
}

