#include <iostream>
#include <entt/entt.hpp>

int main() {
    entt::registry src;
    src.create(entt::entity{0}); // reserve 0
    auto e1 = src.create(); // 1
    src.destroy(e1);
    auto e2 = src.create(); // 1048577 (idx 1, ver 1)
    std::cout << "e2 in src: " << entt::to_integral(e2) << "\n";

    // Clone into dst
    entt::registry dst;
    dst.create(entt::entity{0}); // reserve 0
    
    // Can dst create e2 directly?
    auto cloned = dst.create(e2);
    std::cout << "cloned in dst: " << entt::to_integral(cloned) << ", valid: " << dst.valid(cloned) << "\n";
    std::cout << "is e2 valid in dst: " << dst.valid(e2) << "\n";

    return 0;
}

