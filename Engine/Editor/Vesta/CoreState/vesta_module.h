#pragma once

/**
 * @file vesta_module.h
 * @brief Dış sistemlerin (Fizik, Ses, Render) Vesta editör olaylarına tip bazlı bağlanmasını sağlayan şablon modülü.
 */

#include <functional>
#include <typeinfo>
#include "../../ECS/entity.h"
#include "../Services/editor_events.h"

namespace vesta {

    /**
     * @class vesta_module
     * @brief Alt sistemlerin ECS bileşen silinme kancalarına (hooks) gevşek bağlı (decoupled) şekilde abone olmasını sağlar.
     */
    class vesta_module {
    public:
        /**
         * @brief Belirli bir bileşen silindiğinde çalıştırılacak temizleme fonksiyonunu kaydeder.
         *
         * @tparam TComponent Dinlenecek olan ECS bileşeni türü.
         * @param destroy_fn Bileşene sahip varlık silinirken çağrılacak olan geri çağrım (callback) fonksiyonu.
         */
        template<typename TComponent>
        static void register_remove_hook(std::function<void(entity)> destroy_fn) {
            if (!destroy_fn) {
                return;
            }

            size_t comp_hash = typeid(TComponent).hash_code();
            editor_events::subscribe_on_remove(comp_hash, [fn = std::move(destroy_fn)](entity e, class registry&) {
                fn(e);
                });
        }
    };

} // namespace vesta