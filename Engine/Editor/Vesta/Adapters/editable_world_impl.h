#pragma once
#include "IEditableWorld.h"
#include "../../../ECS/registry.h"       // Projendeki gerçek registry yolu
#include <unordered_map>

namespace vesta {

    class editable_world_impl : public IEditableWorld {
    private:
        registry& m_registry;
        aria::system* m_aria_system;
        bool m_is_dirty = false;

    public:
        editable_world_impl(registry& reg, aria::system* aria_sys);
        virtual ~editable_world_impl() override = default;

        // 🔹 ECS / Sahne Mutasyonları
        virtual entity spawn_entity(const std::string& name = "Empty Entity") override;
        virtual void destroy_entity(entity e) override;
        virtual void set_entity_parent(entity child, entity parent) override;

        // Komponent Operasyonları (Gelecekte component_ui_registry entegrasyonu için)
        virtual void add_component_by_hash(entity e, size_t component_hash) override;
        virtual void remove_component_by_hash(entity e, size_t component_hash) override;
        virtual bool has_component_by_hash(entity e, size_t component_hash) const override;
        virtual void* get_component_raw(entity e, size_t component_hash) override;

        // 🔹 Asset Operasyonları (ARIA Köprüsü)
        virtual std::string resolve_asset_path(const aria::asset_guid& guid) const override;
        virtual aria::asset_guid get_guid_from_path(const std::string& path) override;

        // 🔹 Sahne Durumu (SERA Dirty Flag)
        virtual void mark_scene_dirty() override { m_is_dirty = true; }
        virtual bool is_scene_dirty() const override { return m_is_dirty; }
        void clear_dirty_flag() { m_is_dirty = false; }


        // Component UI
        const ComponentUIInfo* get_component_ui(size_t hash) const override;
        const std::unordered_map<size_t, ComponentUIInfo>& get_component_registry() const override;

        // Asset UI
        const AssetUIInfo* get_asset_ui(const std::string& asset_type) const override;
        const std::unordered_map<std::string, AssetUIInfo>& get_asset_registry() const override;

        // Asset
        std::optional<std::string> get_asset_type(const std::string& path) const override;
        bool asset_exists(const std::string& path) const override;

    };

} // namespace vesta