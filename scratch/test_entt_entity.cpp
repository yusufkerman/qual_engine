#include <iostream>
#include <entt/entt.hpp>

int main() {
    entt::registry reg;
    auto e0 = reg.create();
    auto e1 = reg.create();
    auto e2 = reg.create();

    std::cout << "e0: " << entt::to_integral(e0) << " (idx=" << entt::to_entity(e0) << ", ver=" << entt::to_version(e0) << ")\n";
    std::cout << "e1: " << entt::to_integral(e1) << " (idx=" << entt::to_entity(e1) << ", ver=" << entt::to_version(e1) << ")\n";
    std::cout << "e2: " << entt::to_integral(e2) << " (idx=" << entt::to_entity(e2) << ", ver=" << entt::to_version(e2) << ")\n";

    reg.destroy(e1);
    auto e3 = reg.create();
    std::cout << "After destroying e1, e3: " << entt::to_integral(e3) << " (idx=" << entt::to_entity(e3) << ", ver=" << entt::to_version(e3) << ")\n";

    // Test creating explicit entity
    entt::entity explicit_ent{100};
    if (reg.valid(explicit_ent)) {
        std::cout << "100 valid: true\n";
    } else {
        std::cout << "100 valid: false\n";
    }
    auto e_custom = reg.create(entt::entity{100});
    std::cout << "e_custom: " << entt::to_integral(e_custom) << ", valid: " << reg.valid(e_custom) << "\n";

    return 0;
}

