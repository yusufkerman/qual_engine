#pragma once

/**
 * @file uuid_generator.h
 * @brief Evrensel benzersiz kimlik (UUID) üretimi için yardımcı araçlar.
 */

#define UUID_SYSTEM_GENERATOR

#include "../Externals/uuid/uuid.h" 
#include <random>
#include <string>

 /**
  * @namespace UUIDGenerator
  * @brief Standartlara uygun evrensel benzersiz kimlik (UUID) üretim işlevlerini barındıran isim uzayı.
  */
namespace UUIDGenerator
{
    /**
     * @brief Rastgele donanımsal tohumlama (hardware seed) kullanarak yeni bir UUID v4 dizgesi üretir.
     *
     * @note Bu fonksiyon yüksek entropili std::seed_seq ve std::mt19937 motorlarını kullanarak
     *       çarpışma (collision) ihtimalini minimuma indirir.
     *
     * @return std::string formatında evrensel benzersiz kimlik.
     */
    inline std::string Generate()
    {
        std::random_device rd;
        auto seeds = { rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
        std::seed_seq seed(seeds);
        std::mt19937 engine(seed);

        uuids::uuid_random_generator gen(engine);
        const uuids::uuid id = gen();

        return uuids::to_string(id);
    }
} // namespace UUIDGenerator