#pragma once

/**
 * @file component_pool.h
 * @brief Belirli bir tipteki bileşenleri barındıran jenerik havuz sınıfı.
 */

#include <unordered_map>
#include <typeindex>
#include <memory>
#include "component_pool_interface.h"
#include "registry.h"

 /**
  * @class component_pool
  * @brief Varlık kimlikleriyle bileşen verilerini eşleştiren tip-güvenli depolama alanı.
  * @tparam T Bileşen tipi.
  */
template<typename T>
class component_pool : public component_pool_interface
{
public:
    std::unordered_map<entity, T> data; ///< Entity-Bileşen eşleme tablosu.

    virtual void clone_entity(entity e, registry& dst, const registry& src) const override
    {
        if (const auto* comp = src.get_component<T>(e))
        {
            dst.add_component<T>(e, *comp);
        }
    }
};