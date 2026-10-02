#pragma once

/**
 * @file component_operation_registry.h
 * @brief ECS bileşenleri (components) için tip silinmiş (type-erased) ekleme, silme ve sorgulama operasyonlarını yöneten kayıt defteri.
 */

#include <functional>
#include <unordered_map>
#include "../../ECS/entity.h"

 // Forward declaration
class registry;

namespace vesta
{
    /**
     * @typedef ComponentAddFn
     * @brief Belirli bir entity'ye ilgili bileşeni ekleyen callback fonksiyonu.
     */
    using ComponentAddFn = std::function<void(entity, registry&)>;

    /**
     * @typedef ComponentRemoveFn
     * @brief İlgili bileşeni belirli bir entity'den silen callback fonksiyonu.
     */
    using ComponentRemoveFn = std::function<void(entity, registry&)>;

    /**
     * @typedef ComponentHasFn
     * @brief Belirli bir entity'nin ilgili bileşene sahip olup olmadığını kontrol eden callback fonksiyonu.
     */
    using ComponentHasFn = std::function<bool(entity, registry&)>;

    /**
     * @struct ComponentOperationInfo
     * @brief Bir bileşen tipi için gerekli olan tüm operasyon (add, remove, has) callback'lerini barındırır.
     */
    struct ComponentOperationInfo
    {
        ComponentAddFn add_fn;
        ComponentRemoveFn remove_fn;
        ComponentHasFn has_fn;
    };

    /**
     * @class component_operation_registry
     * @brief Editör arayüzünün, bileşen tiplerini (compile-time) bilmeden (runtime'da hash üzerinden) ECS üzerinde işlem yapmasını sağlayan statik merkez.
     */
    class component_operation_registry
    {
    public:
        /**
         * @brief Yeni bir bileşen operasyon grubunu sisteme kaydeder.
         *
         * @param hash Bileşenin benzersiz tip kimliği (type hash).
         * @param info Bileşene ait ekleme, silme ve sorgulama fonksiyonları.
         */
        static void register_component(size_t hash, const ComponentOperationInfo& info)
        {
            get_registry_internal()[hash] = info;
        }

        /**
         * @brief Verilen hash değerine sahip bileşenin operasyon bilgilerini arar.
         *
         * @param hash Aranacak bileşenin tip kimliği.
         * @return const ComponentOperationInfo* Bulunursa bilgi yapısına işaretçi, bulunamazsa nullptr.
         */
        static const ComponentOperationInfo* find(size_t hash)
        {
            auto& registry = get_registry_internal();
            auto it = registry.find(hash);

            if (it == registry.end())
            {
                return nullptr;
            }

            return &it->second;
        }

        /**
         * @brief Kayıtlı tüm bileşen operasyon haritasını döndürür.
         *
         * @return const std::unordered_map<size_t, ComponentOperationInfo>& Salt okunur harita referansı.
         */
        static const std::unordered_map<size_t, ComponentOperationInfo>& get_registry()
        {
            return get_registry_internal();
        }

    private:
        /**
         * @brief Meyer's Singleton deseni ile operasyon haritasının tek ve güvenli bir örneğini (instance) oluşturur.
         */
        static std::unordered_map<size_t, ComponentOperationInfo>& get_registry_internal()
        {
            static std::unordered_map<size_t, ComponentOperationInfo> registry;
            return registry;
        }
    };

} // namespace vesta