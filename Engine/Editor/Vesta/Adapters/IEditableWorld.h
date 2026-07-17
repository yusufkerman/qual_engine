#pragma once
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include "../../ECS/entity.h"       // Senin projedeki entity tanımın
#include "../../../Engine/Assets/aria.h"         // aria::asset_guid için projedeki yolun
#include "../../Renderer/material.h" // material_handle vb. yapılar için
#include "../Services/asset_ui_registry.h"
#include "../Services/component_ui_registry.h"

namespace vesta {

    class IEditableWorld {
    public:
        virtual ~IEditableWorld() = default;

        // ==========================================
        // 🔹 ECS / SAHNE MUTASYONLARI (SERA tetikleyici)
        // ==========================================
        virtual entity spawn_entity(const std::string& name = "Empty Entity") = 0;
        virtual void destroy_entity(entity e) = 0;

        // Hiyerarşi (Parent-Child) Yönetimi
        virtual void set_entity_parent(entity child, entity parent) = 0;

        // Komponent Operasyonları
        virtual void add_component_by_hash(entity e, size_t component_hash) = 0;
        virtual void remove_component_by_hash(entity e, size_t component_hash) = 0;
        virtual bool has_component_by_hash(entity e, size_t component_hash) const = 0;
        virtual void* get_component_raw(entity e, size_t component_hash) = 0;

        // ==========================================
        // 🔹 ASSET / CONFIG OPERASYONLARI (ARIA tetikleyici)
        // ==========================================
        virtual std::string resolve_asset_path(const aria::asset_guid& guid) const = 0;
        virtual aria::asset_guid get_guid_from_path(const std::string& path) = 0;

        // ==========================================
        // 🔹 SAHNE DURUMU (SERA Dirty Flag)
        // ==========================================
        virtual void mark_scene_dirty() = 0;
        virtual bool is_scene_dirty() const = 0;


        // Component UI
        virtual const ComponentUIInfo*
            get_component_ui(size_t hash) const = 0;

        virtual const std::unordered_map<size_t, ComponentUIInfo>&
            get_component_registry() const = 0;

        // Asset UI
        virtual const AssetUIInfo*
            get_asset_ui(const std::string& asset_type) const = 0;

        virtual const std::unordered_map<std::string, AssetUIInfo>&
            get_asset_registry() const = 0;

        // Asset
        virtual std::optional<std::string>
            get_asset_type(const std::string& path) const = 0;

        virtual bool
            asset_exists(const std::string& path) const = 0;

    };

} // vesta