#pragma once

/**
 * @file editable_world_impl.h
 * @brief IEditableWorld adaptör arayüzünün somut sınıf tanımını içerir.
 */

#include "IEditableWorld.h"
#include "../../../ECS/registry.h"
#include <unordered_map>
#include <optional>
#include <string>

namespace aria {
    class system;
}

namespace vesta {

    /**
     * @class editable_world_impl
     * @brief Vesta editörünün sahne mutasyonlarını ve varlık sorgularını motor sistemlerine ileten adaptör sınıfı.
     */
    class editable_world_impl : public IEditableWorld {
    private:
        registry& m_registry;
        aria::system* m_aria_system;
        bool m_is_dirty = false;

    public:
        /**
         * @brief Somut dünya adaptörünü başlatır.
         * @param reg İşlem yapılacak ECS kayıt defteri referansı.
         * @param aria_sys Varlık boru hattı ve GUID yönetimi için Aria sistemi işaretçisi.
         */
        editable_world_impl(registry& reg, aria::system* aria_sys);
        ~editable_world_impl() override = default;

        // ==========================================
        // SAHNE VE VARLIK MUTASYONLARI
        // ==========================================

        entity spawn_entity(const std::string& name = "Empty Entity") override;
        void destroy_entity(entity e) override;
        void set_entity_parent(entity child, entity parent) override;

        // ==========================================
        // BİLEŞEN OPERASYONLARI
        // ==========================================

        void add_component_by_hash(entity e, size_t component_hash) override;
        void remove_component_by_hash(entity e, size_t component_hash) override;
        bool has_component_by_hash(entity e, size_t component_hash) const override;
        void* get_component_raw(entity e, size_t component_hash) override;

        // ==========================================
        // VARLIK (ASSET) OPERASYONLARI
        // ==========================================

        std::string resolve_asset_path(const aria::asset_guid& guid) const override;
        aria::asset_guid get_guid_from_path(const std::string& path) override;
        std::optional<std::string> get_asset_type(const std::string& path) const override;
        bool asset_exists(const std::string& path) const override;

        // ==========================================
        // SAHNE DURUMU (DIRTY STATE)
        // ==========================================

        void mark_scene_dirty() override { m_is_dirty = true; }
        bool is_scene_dirty() const override { return m_is_dirty; }
        void clear_dirty_flag() { m_is_dirty = false; }

        // ==========================================
        // ARAYÜZ KAYIT ERİŞİMCİLERİ (REGISTRY ACCESSORS)
        // ==========================================

        const ComponentUIInfo* get_component_ui(size_t hash) const override;
        const std::unordered_map<size_t, ComponentUIInfo>& get_component_registry() const override;

        const AssetUIInfo* get_asset_ui(const std::string& asset_type) const override;
        const std::unordered_map<std::string, AssetUIInfo>& get_asset_registry() const override;
    };

} // namespace vesta