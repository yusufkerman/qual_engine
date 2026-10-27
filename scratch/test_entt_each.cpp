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

    auto view = reg.view<position>();
    std::cout << "Testing view.each():\n";
    for (auto [e, pos] : view.each()) {
        std::cout << "ent: " << entt::to_integral(e) << " (" << pos.x << ", " << pos.y << ")\n";
        pos.x += 1.0f;
    }

    std::cout << "Testing view.each() const/ref modification:\n";
    for (auto [e, pos] : view.each()) {
        std::cout << "ent: " << entt::to_integral(e) << " (" << pos.x << ", " << pos.y << ")\n";
    }

    return 0;
}

