#pragma once

#include "material.h"
#include <unordered_map>

class material_manager
{
public:

    unsigned int create(const material& mat);
    const std::unordered_map<unsigned int, material>& get_all_materials() const { return materials; }

    material& get(unsigned int id);
    const material& get(unsigned int id) const;

private:
    std::unordered_map<unsigned int, material> materials;
    unsigned int nextID = 0;
};