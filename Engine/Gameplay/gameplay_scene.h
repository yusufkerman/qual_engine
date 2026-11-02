#pragma once

/**
 * @file gameplay_scene.h
 * @brief Sahne varlıklarını arama, oluşturma ve yok etme işlemlerini yöneten yüksek seviye sahne servisi.
 */

#include <string>
#include <glm/glm.hpp>
#include "gameplay_entity.h"

namespace gameplay
{
    /**
     * @struct raycast_hit
     * @brief Işın sorgusu (raycast) neticesinde temas edilen nesne ve yüzey verileri.
     */
    struct raycast_hit
    {
        bool has_hit{ false };
        gameplay_entity entity;
        glm::vec3 point{ 0.0f };
        glm::vec3 normal{ 0.0f };
        float distance{ 0.0f };

        explicit operator bool() const noexcept { return has_hit; }
    };

    /**
     * @class scene_service
     * @brief Aktif sahnedeki varlıkların aranması, oluşturulması, ışın sorguları ve yaşam döngüsünü yöneten statik servis.
     */
    class scene_service
    {
    public:
        /**
         * @brief Sahnede belirtilen ada (name_component) sahip ilk varlığı arar.
         * @param name Aranacak varlık adı.
         * @return Bulunursa geçerli gameplay_entity, bulunamazsa geçersiz nesne döner.
         */
        [[nodiscard]] static gameplay_entity find_entity(const std::string& name);

        /**
         * @brief Ham kimlikten (ID) bir gameplay_entity tutucusu üretir.
         * @param id Varlık kimliği.
         */
        [[nodiscard]] static gameplay_entity get_entity(entity id);

        /**
         * @brief Sahnede yeni ve boş bir varlık oluşturur.
         * @param name Varlığa atanacak opsiyonel isim.
         * @return Yeni oluşturulan varlığın tutucusu.
         */
        static gameplay_entity create_entity(const std::string& name = "");

        /**
         * @brief Belirtilen varlığı sahneden kaldırır.
         * @param id Silinecek varlık kimliği.
         */
        static void destroy(entity id);

        /**
         * @brief Belirtilen varlığı sahneden kaldırır.
         * @param ent Silinecek varlık tutucusu.
         */
        static void destroy(gameplay_entity ent);

        /**
         * @brief Sahnede aktif olan birincil kamerayı döner.
         */
        [[nodiscard]] static gameplay_entity get_active_camera();

        /**
         * @brief Fizik sahnesinde belirtilen doğrultuda ışın fırlatarak ilk çarpan nesneyi döner.
         * @param origin Başlangıç konumu.
         * @param direction Işın yönü.
         * @param max_distance Maksimum mesafe (varsayılan: 1000m).
         * @param ignore_entity Çarpışmada yok sayılacak varlık kimliği (varsayılan: 0).
         * @return Çarpışma bilgisi (raycast_hit).
         */
        [[nodiscard]] static raycast_hit raycast(const glm::vec3& origin, const glm::vec3& direction, float max_distance = 1000.0f, entity ignore_entity = 0);
    };

    /**
     * @namespace physics
     * @brief Fizik alt sistemi için doğrudan erişilebilir ergonomik fonksiyonlar.
     */
    namespace physics
    {
        [[nodiscard]] inline raycast_hit raycast(const glm::vec3& origin, const glm::vec3& direction, float max_distance = 1000.0f, entity ignore_entity = 0)
        {
            return scene_service::raycast(origin, direction, max_distance, ignore_entity);
        }
    }
}
