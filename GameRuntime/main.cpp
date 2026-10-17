/**
 * @file main.cpp
 * @brief Standalone (Editörsüz) Bağımsız Oyun Yürütücüsü (Player).
 */

#include <Engine/Runtime/game_runtime.h>
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << std::unitbuf;
    return runtime::game_runtime::run_standalone(argc, argv);
}

