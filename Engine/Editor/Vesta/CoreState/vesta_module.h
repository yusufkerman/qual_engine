// 📁 Dosya Yolu: Vesta/Core/vesta_module.h
#pragma once
#include <functional>
#include <typeinfo>
#include "../../ECS/entity.h"
#include "../Services/editor_events.h"

namespace vesta {
    class vesta_module {
    public:
        // Modülün içi tamamen jenerik, hiçbir bileşeni include etmiyor!
        template<typename TComponent>
        static void register_remove_hook(std::function<void(entity)> destroy_fn) {
            if (!destroy_fn) return;

            size_t comp_hash = typeid(TComponent).hash_code();
            editor_events::subscribe_on_remove(comp_hash, [destroy_fn](entity e, class registry&) {
                destroy_fn(e);
                });
        }
    };
}