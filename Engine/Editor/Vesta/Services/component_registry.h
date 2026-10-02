#pragma once

/**
 * @file component_registry.h
 * @brief (LEGACY) ECS bileşenleri için tip silinmiş (type-erased) operasyon kayıt defteri taslağı.
 * @deprecated DİKKAT: Bu dosya 'component_operation_registry.h' dosyasının eski/kopya versiyonudur.
 * Sistemde mimari karmaşaya yol açmamak adına projeden tamamen silinmesi ve referanslarının
 * 'component_operation_registry' sınıfına geçirilmesi tavsiye edilir.
 */

#include <functional>
#include <unordered_map>
#include "../../ECS/entity.h"

class registry;

namespace vesta
{
    /** @deprecated Lütfen component_operation_registry içindeki tanımları kullanın. */
    using ComponentAddFn = std::function<void(entity, registry&)>;

    /** @deprecated Lütfen component_operation_registry içindeki tanımları kullanın. */
    using ComponentRemoveFn = std::function<void(entity, registry&)>;

    /** @deprecated Lütfen component_operation_registry içindeki tanımları kullanın. */
    using ComponentHasFn = std::function<bool(entity, registry&)>;

    /**
     * @struct ComponentInfo
     * @brief (LEGACY) Bileşen operasyon bilgileri.
     * @deprecated Yerine ComponentOperationInfo kullanın.
     */
    struct ComponentInfo
    {
        ComponentAddFn add_fn;
        ComponentRemoveFn remove_fn;
        ComponentHasFn has_fn;
    };

    /**
     * @class component_registry
     * @brief (LEGACY) Eski bileşen operasyon yöneticisi.
     * @deprecated Yerine component_operation_registry kullanın.
     */
    class component_registry
    {
    public:
        static void register_component(size_t hash, const ComponentInfo& info)
        {
            get_components()[hash] = info;
        }

        static const ComponentInfo* find(size_t hash)
        {
            auto& registry = get_components();
            auto it = registry.find(hash);

            if (it == registry.end())
            {
                return nullptr;
            }

            return &it->second;
        }

    private:
        static std::unordered_map<size_t, ComponentInfo>& get_components()
        {
            static std::unordered_map<size_t, ComponentInfo> components;
            return components;
        }
    };

} // namespace vesta