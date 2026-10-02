#pragma once

/**
 * @file editor_events.h
 * @brief Editör içinde gerçekleşen ECS bileşen değişikliklerini dış sistemlere (Fizik, Render vb.) bildiren olay (event) mekanizması.
 */

#include <functional>
#include <unordered_map>
#include <vector>
#include "../../ECS/entity.h"

 // Forward declaration
class registry;

namespace vesta {

    /**
     * @class editor_events
     * @brief Bileşen silinmesi gibi editör eylemlerini dinlemek (subscribe) ve tetiklemek (dispatch) için kullanılan statik olay dağıtıcı.
     */
    class editor_events {
    public:
        /**
         * @typedef ComponentCallback
         * @brief Bir bileşen eylemi gerçekleştiğinde tetiklenecek geri çağrım (callback) fonksiyonu.
         */
        using ComponentCallback = std::function<void(entity, registry&)>;

        /**
         * @brief Belirli bir bileşen silindiğinde haberdar olmak için (Observer) bir dinleyici kaydeder.
         * @note Dış sistemlerin (Fizik, Ses vb.) editör işlemlerine gevşek bağlı (decoupled) şekilde tepki vermesini sağlar.
         *
         * @param comp_hash Dinlenecek bileşenin benzersiz tip kimliği (hash).
         * @param callback Bileşen silindiğinde çalıştırılacak fonksiyon.
         */
        static void subscribe_on_remove(size_t comp_hash, ComponentCallback callback) {
            get_listeners()[comp_hash].push_back(std::move(callback));
        }

        /**
         * @brief Bir bileşen silindiğinde, o bileşen tipini dinleyen tüm sistemlere bildirim (event) gönderir.
         * @note Genellikle 'editable_world_impl' gibi merkezi dünya yöneticileri tarafından, bileşen gerçekten silinmeden hemen önce çağrılır.
         *
         * @param comp_hash Silinen bileşenin tip kimliği.
         * @param e Bileşenin silindiği varlık (entity).
         * @param reg İşlemin gerçekleştiği ECS kayıt defteri.
         */
        static void dispatch_on_remove(size_t comp_hash, entity e, registry& reg) {
            auto& listeners = get_listeners();
            auto it = listeners.find(comp_hash);

            if (it != listeners.end()) {
                for (auto& callback : it->second) {
                    callback(e, reg);
                }
            }
        }

    private:
        /**
         * @brief Silinme olaylarını dinleyenlerin listesini (haritasını) tutan statik merkez.
         */
        static std::unordered_map<size_t, std::vector<ComponentCallback>>& get_listeners() {
            static std::unordered_map<size_t, std::vector<ComponentCallback>> listeners;
            return listeners;
        }
    };

} // namespace vesta