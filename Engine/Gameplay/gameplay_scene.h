#pragma once

/**
 * @file gameplay_scene.h
 * @brief Sahne varlıklarını arama, oluşturma ve yok etme işlemlerini yöneten yüksek seviye sahne servisi.
 */

#include <string>
#include "gameplay_entity.h"

namespace gameplay
{
    /**
     * @class scene_service
     * @brief Aktif sahnedeki varlıkların aranması, oluşturulması ve yaşam döngüsünü yöneten statik servis.
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
    };
}

