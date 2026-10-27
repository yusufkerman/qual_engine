#include <iostream>
#include <entt/entt.hpp>

struct position { float x, y; };

int main() {
    entt::registry reg1;
    auto e = reg1.create();
    reg1.emplace<position>(e, 1.0f, 2.0f);

    // Test move
    entt::registry reg2 = std::move(reg1);
    std::cout << "reg2 valid: " << reg2.valid(e) << "\n";

    // Test copy
    entt::registry reg3 = reg2; // Does this compile?
    std::cout << "reg3 valid: " << reg3.valid(e) << "\n";
    return 0;
}
