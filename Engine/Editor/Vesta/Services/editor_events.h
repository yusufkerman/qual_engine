#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include "../../ECS/entity.h"

namespace vesta {

    class editor_events {
    public:
        // Herhangi bir component silindiğinde tetiklenecek fonksiyon imzası
        using ComponentCallback = std::function<void(entity, class registry&)>;

        // Dış sistemlerin (Fizik vb.) Vesta'yı dinlemesini sağlayan fonksiyon
        static void subscribe_on_remove(size_t comp_hash, ComponentCallback callback) {
            get_listeners()[comp_hash].push_back(callback);
        }

        // Vesta'nın (editable_world_impl) içeride bu olayları patlatmasını sağlayan fonksiyon
        static void dispatch_on_remove(size_t comp_hash, entity e, class registry& reg) {
            auto& listeners = get_listeners();
            auto it = listeners.find(comp_hash);
            if (it != listeners.end()) {
                for (auto& callback : it->second) {
                    callback(e, reg); // Dinleyen herkese haber ver
                }
            }
        }

    private:
        static std::unordered_map<size_t, std::vector<ComponentCallback>>& get_listeners() {
            static std::unordered_map<size_t, std::vector<ComponentCallback>> listeners;
            return listeners;
        }
    };

} // namespace vesta