#pragma once

/**
 * @file IEditableWorld.h
 * @brief Vesta editörünün sahne mutasyonlarını ve varlık işlemlerini soyutlayan temel arayüz.
 */

#include "../../ECS/entity.h"
#include "../../../Engine/Assets/aria.h"
#include "../Services/asset_ui_registry.h"
#include "../Services/component_ui_registry.h"
#include <string>
#include <optional>
#include <unordered_map>

namespace vesta {

    /**
     * @class IEditableWorld
     * @brief Editör komutları, görünüm modelleri ve paneller için düzenlenebilir dünya sözleşmesi.
     * @note Doğrudan motor çekirdeğine bağlanmak yerine bu adaptör arayüzü kullanılır.
     */
    class IEditableWorld {
    public:
        virtual ~IEditableWorld() = default;

        // ==========================================
        // SAHNE VE VARLIK MUTASYONLARI
        // ==========================================

        /**
         * @brief Sahnede yeni bir boş varlık oluşturur.
         * @param name Varlığa atanacak varsayılan isim.
         * @return entity Oluşturulan varlığın kimliği.
         */
        virtual entity spawn_entity(const std::string& name = "Empty Entity") = 0;

        /**
         * @brief Belirtilen varlığı ve bağlı hiyerarşisini sahneden kaldırır.
         * @param e Silinecek varlığın kimliği.
         */
        virtual void destroy_entity(entity e) = 0;

        /**
         * @brief Bir varlığın ebeveynini (parent) günceller.
         * @param child Ebeveyni değiştirilecek alt varlık.
         * @param parent Yeni ebeveyn varlık (0 ise kök dizin).
         */
        virtual void set_entity_parent(entity child, entity parent) = 0;

        // ==========================================
        // BİLEŞEN OPERASYONLARI
        // ==========================================

        /**
         * @brief Tip özeti (hash) üzerinden varlığa bileşen ekler.
         */
        virtual void add_component_by_hash(entity e, size_t component_hash) = 0;

        /**
         * @brief Tip özeti (hash) üzerinden varlıktan bileşeni kaldırır.
         */
        virtual void remove_component_by_hash(entity e, size_t component_hash) = 0;

        /**
         * @brief Varlığın belirtilen bileşene sahip olup olmadığını sorgular.
         */
        virtual bool has_component_by_hash(entity e, size_t component_hash) const = 0;

        /**
         * @brief Bileşenin ham bellek işaretçisini döndürür.
         */
        virtual void* get_component_raw(entity e, size_t component_hash) = 0;

        // ==========================================
        // VARLIK (ASSET) OPERASYONLARI
        // ==========================================

        /**
         * @brief GUID değerini fiziksel disk yoluna çözümler.
         */
        virtual std::string resolve_asset_path(const aria::asset_guid& guid) const = 0;

        /**
         * @brief Verilen dosya yoluna karşılık gelen varlık GUID'sini döndürür veya oluşturur.
         */
        virtual aria::asset_guid get_guid_from_path(const std::string& path) = 0;

        /**
         * @brief Dosyanın varlık türünü belirler.
         */
        virtual std::optional<std::string> get_asset_type(const std::string& path) const = 0;

        /**
         * @brief Dosyanın disk üzerinde var olup olmadığını doğrular.
         */
        virtual bool asset_exists(const std::string& path) const = 0;

        // ==========================================
        // SAHNE DURUMU (DIRTY STATE)
        // ==========================================

        /**
         * @brief Sahneyi kaydedilmemiş değişiklikler barındırıyor olarak işaretler.
         */
        virtual void mark_scene_dirty() = 0;

        /**
         * @brief Sahnede kaydedilmemiş değişiklik olup olmadığını kontrol eder.
         */
        virtual bool is_scene_dirty() const = 0;

        // ==========================================
        // ARAYÜZ KAYIT ERİŞİMCİLERİ (REGISTRY ACCESSORS)
        // ==========================================

        virtual const ComponentUIInfo* get_component_ui(size_t hash) const = 0;
        virtual const std::unordered_map<size_t, ComponentUIInfo>& get_component_registry() const = 0;

        virtual const AssetUIInfo* get_asset_ui(const std::string& asset_type) const = 0;
        virtual const std::unordered_map<std::string, AssetUIInfo>& get_asset_registry() const = 0;

        virtual aria::system* get_aria_system() = 0;
    };

} // namespace vesta