#include <iostream>
#include <entt/entt.hpp>

struct position {
    float x, y;
};

int main() {
    entt::registry reg;
    auto e1 = reg.create();
    reg.emplace<position>(e1, 10.0f, 20.0f);

    auto view = reg.view<position>();
    // Try auto& [e, p]
    for (auto& [e, p] : view.each()) {
        std::cout << "auto&&: " << entt::to_integral(e) << " (" << p.x << ", " << p.y << ")\n";
    }

    return 0;
}
