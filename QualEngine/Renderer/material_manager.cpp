#include "material_manager.h"

unsigned int material_manager::create(const material& mat)
{
    unsigned int id = nextID++;
    materials[id] = mat;
    return id;
}

material& material_manager::get(unsigned int id)
{
    return materials.at(id);
}

const material& material_manager::get(unsigned int id) const
{
    return materials.at(id);
}