/**
 * @file material_manager.cpp
 * @brief Eski materyal yöneticisi uygulaması.
 */

#include "material_manager.h"
#include <iostream>

material material_manager::s_defaultMaterial{};

unsigned int material_manager::create(const material& mat)
{
    const unsigned int id = m_nextID++;
    m_materials[id] = mat;
    return id;
}

material& material_manager::get(unsigned int id)
{
    const auto it = m_materials.find(id);
    if (it != m_materials.end())
    {
        return it->second;
    }

    std::cerr << "[MaterialManager] Gecersiz materyal kimligi: " << id << ", varsayilan donuluyor.\n";
    return s_defaultMaterial;
}

const material& material_manager::get(unsigned int id) const
{
    const auto it = m_materials.find(id);
    if (it != m_materials.end())
    {
        return it->second;
    }

    std::cerr << "[MaterialManager] Gecersiz materyal kimligi: " << id << ", varsayilan donuluyor.\n";
    return s_defaultMaterial;
}

void material_manager::clear() noexcept
{
    m_materials.clear();
    m_nextID = 1;
}