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

    for (auto [ent, p] : storage.each()) {
        std::cout << "storage.each(): " << entt::to_integral(ent) << " (" << p.x << ", " << p.y << ")\n";
    }

    auto view = reg.view<position>();
    for (auto [ent, p] : view.each()) {
        std::cout << "view.each(): " << entt::to_integral(ent) << " (" << p.x << ", " << p.y << ")\n";
    }

    return 0;
}

