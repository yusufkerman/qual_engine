#include <iostream>

struct MyAdapter {
    int x = 5;
};

MyAdapter get_adapter() { return MyAdapter{}; }

int main() {
    // auto& a = get_adapter(); // C2440!
    const auto& b = get_adapter(); // OK
    return 0;
}

