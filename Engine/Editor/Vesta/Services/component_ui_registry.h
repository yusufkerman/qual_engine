#pragma once

/**
 * @file component_ui_registry.h
 * @brief ECS bileşenlerinin (components) Inspector (Özellikler) panelinde nasıl çizileceğini belirleyen arayüz kayıt sistemi.
 */

#include <functional>
#include <string>
#include <unordered_map>
#include "../../ECS/entity.h"

 // Forward declaration
class registry;

namespace vesta
{
    /**
     * @typedef ComponentRenderFn
     * @brief Bir ECS bileşeninin arayüz üzerinde (ImGui ile) çizilmesini sağlayan geri çağrım (callback) fonksiyonu.
     * @param entity Çizilecek bileşene sahip olan varlık.
     * @param registry Varlığın bileşenlerine erişim sağlayan ECS kayıt defteri.
     */
    using ComponentRenderFn = std::function<void(entity, registry&)>;

    /**
     * @struct ComponentUIInfo
     * @brief Bir bileşenin arayüz (UI) tarafında nasıl temsil edileceğini ve çizileceğini barındıran bilgi yapısı.
     */
    struct ComponentUIInfo
    {
        std::string display_name;       ///< Bileşenin Inspector panelinde görünecek olan başlığı (Örn: "Transform", "Mesh Renderer").
        std::string category = "General"; ///< Bileşenin "Add Component" menüsündeki kategorisi (Örn: "Rendering", "Physics", "UI", "Animation", "Scripting").
        ComponentRenderFn render_fn;    ///< Bileşeni ekrana çizdirecek olan (ImGui) fonksiyon.
    };

    /**
     * @class component_ui_registry
     * @brief Bileşen tipleri (hash) ile onların UI çizim fonksiyonlarını eşleştiren statik merkez.
     * @note UI katmanı ile ECS katmanını birbirinden izole etmek için fonksiyonel (callback) tasarlanmıştır.
     */
    class component_ui_registry
    {
    public:
        /**
         * @brief Yeni bir bileşen için arayüz çizim bilgisini sisteme kaydeder.
         *
         * @param hash Bileşenin benzersiz tip kimliği (type hash).
         * @param info Bileşenin ekranda nasıl görüneceğini ve çizileceğini belirten bilgi yapısı.
         */
        static void register_component(size_t hash, const ComponentUIInfo& info)
        {
            get_components()[hash] = info;
        }

        /**
         * @brief Verilen hash değerine sahip bileşenin arayüz çizim bilgilerini arar.
         *
         * @param hash Aranacak bileşenin tip kimliği.
         * @return const ComponentUIInfo* Bulunursa bilgi yapısına işaretçi, bulunamazsa nullptr.
         */
        static const ComponentUIInfo* find(size_t hash)
        {
            auto& registry = get_components();
            auto it = registry.find(hash);

            if (it == registry.end())
            {
                return nullptr;
            }

            return &it->second;
        }

        /**
         * @brief Kayıtlı tüm bileşen UI çizici haritasını döndürür.
         *
         * @return const std::unordered_map<size_t, ComponentUIInfo>& Salt okunur harita referansı.
         */
        static const std::unordered_map<size_t, ComponentUIInfo>& get_registry()
        {
            return get_components();
        }

    private:
        /**
         * @brief Meyer's Singleton deseni ile haritanın tek ve güvenli bir örneğini (instance) oluşturur.
         */
        static std::unordered_map<size_t, ComponentUIInfo>& get_components()
        {
            static std::unordered_map<size_t, ComponentUIInfo> components;
            return components;
        }
    };

} // namespace vesta