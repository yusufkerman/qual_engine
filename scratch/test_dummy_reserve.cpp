#include <iostream>
#include <entt/entt.hpp>

int main() {
    entt::registry reg;
    // Reserve index 0
    auto dummy = reg.create();
    std::cout << "Dummy 0 created: " << entt::to_integral(dummy) << "\n";

    auto e1 = reg.create();
    auto e2 = reg.create();
    std::cout << "First user entity: " << entt::to_integral(e1) << " (idx=" << entt::to_entity(e1) << ")\n";
    std::cout << "Second user entity: " << entt::to_integral(e2) << " (idx=" << entt::to_entity(e2) << ")\n";

    reg.destroy(e1);
    auto e3 = reg.create();
    std::cout << "After destroying e1, recycled e3: " << entt::to_integral(e3) << " (idx=" << entt::to_entity(e3) << ")\n";

    // Check iterating all entities
    auto view = reg.view<entt::entity>();
    std::cout << "Iterating all entities in view:\n";
    for (auto ent : view) {
        std::cout << "  ent: " << entt::to_integral(ent) << "\n";
    }

    return 0;
}

