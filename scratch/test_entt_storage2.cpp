#include <iostream>
#include <entt/entt.hpp>

struct position {
    float x, y;
};

int main() {
    entt::registry reg;
    auto e1 = reg.create();
    auto e2 = reg.create();
    reg.emplace<position>(e1, 10.0f, 20.0f);
    reg.emplace<position>(e2, 30.0f, 40.0f);

    auto& storage = reg.storage<position>();
    std::cout << "storage.size(): " << storage.size() << "\n";
    std::cout << "storage.empty(): " << storage.empty() << "\n";

    // Testing storage.each()
    storage.each([](entt::entity ent, position& p) {
        std::cout << "each: " << entt::to_integral(ent) << " (" << p.x << ", " << p.y << ")\n";
    });

    return 0;
}

