#pragma once

/**
 * @file component_pool_interface.h
 * @brief Bileşen havuzları için soyut arayüz tanımları.
 */

#include "entity.h"

class registry;

/**
 * @struct component_pool_interface
 * @brief Tüm tip-güvenli bileşen havuzlarının türediği ortak arayüz. Kopyalama (Clone) gibi jenerik işlemleri zorunlu kılar.
 */
struct component_pool_interface
{
    virtual ~component_pool_interface() = default;

    /**
     * @brief Belirtilen varlığın bileşenini kaynak kayıt defterinden hedef kayıt defterine kopyalar.
     */
    virtual void clone_entity(entity e, registry& dst, const registry& src) const = 0;
};