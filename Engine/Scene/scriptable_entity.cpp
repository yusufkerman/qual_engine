/**
 * @file scriptable_entity.cpp
 * @brief Yerel betikler için yardımcı işlevlerin ve varlık arama mantığının uygulanması.
 */

#include "scriptable_entity.h"
#include "../ECS/components.h" // name_component erişimi için gerekli

entity scriptable_entity::find_by_name(const std::string& name)
{
    registry* reg = get_registry();
    if (!reg)
    {
        return 0;
    }

    for (const entity e : reg->get_all_entities())
    {
        const auto* n = reg->get_component<name_component>(e);
        if (n && n->name == name)
        {
            return e;
        }
    }

    return 0;
}