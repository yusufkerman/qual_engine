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
    std::cout << "storage.contains(e1): " << storage.contains(e1) << "\n";

    // How to iterate storage?
    for (auto it = storage.cbegin(); it != storage.cend(); ++it) {
        auto ent = *it;
        auto& comp = storage.get(ent);
        std::cout << "storage ent: " << entt::to_integral(ent) << " (" << comp.x << ", " << comp.y << ")\n";
    }

    return 0;
}

