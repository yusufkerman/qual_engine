#include "scriptable_entity.h"


entity scriptable_entity::find_by_name(const std::string& name)
{
    auto* reg = get_registry();
    if (!reg) return 0;

    for (auto e : reg->get_all_entities())
    {
        auto* n = reg->get_component<name_component>(e);
        if (n && n->name == name)
            return e;
    }

    return 0;
}