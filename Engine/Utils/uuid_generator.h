#pragma once
#define UUID_SYSTEM_GENERATOR
#include "../Externals/uuid/uuid.h" 
#include <random>
#include <string>

namespace UUIDGenerator
{
    inline std::string Generate()
    {
        std::random_device rd;
        auto seeds = { rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
        std::seed_seq seed(seeds);
        std::mt19937 engine(seed);

        uuids::uuid_random_generator gen(engine);
        uuids::uuid id = gen();

        return uuids::to_string(id);
    }
}