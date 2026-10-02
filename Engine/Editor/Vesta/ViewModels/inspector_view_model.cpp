/**
 * @file inspector_view_model.cpp
 * @brief Inspector (Özellikler) görünüm modelinin mantıksal uygulaması.
 */

#include "inspector_view_model.h"

namespace vesta
{
    inspector_view_model::inspector_view_model(
        IEditableWorld& world,
        registry& reg,
        editor_selection_manager& selection)
        : m_world(world)
        , m_registry(reg)
        , m_selection(selection)
    {
    }

    void inspector_view_model::update()
    {
        if (m_selection.has_entity_selection())
        {
            m_mode = inspector_mode::entity;
            return;
        }

        if (m_selection.has_asset_selection())
        {
            m_mode = inspector_mode::asset;
            return;
        }

        m_mode = inspector_mode::none;
    }

    void inspector_view_model::refresh()
    {
        update();
    }

    inspector_mode inspector_view_model::get_mode() const
    {
        return m_mode;
    }

    std::vector<add_component_entry> inspector_view_model::get_addable_components() const
    {
        std::vector<add_component_entry> out;

        if (!has_selected_entity())
            return out;

        entity e = get_selected_entity();

        // UI'ı olan ve varlıkta henüz bulunmayan bileşenleri filtrele
        for (const auto& [hash, info] : m_world.get_component_registry())
        {
            if (m_world.has_component_by_hash(e, hash))
                continue;

            out.push_back({ hash, &info });
        }

        return out;
    }

    asset_inspector_data inspector_view_model::get_asset_data() const
    {
        asset_inspector_data data;

        if (!has_selected_asset())
            return data;

        data.asset_path = get_selected_asset();

        auto type = m_world.get_asset_type(data.asset_path);

        if (!type)
            return data;

        data.asset_type = *type;
        data.ui = m_world.get_asset_ui(*type);

        return data;
    }

    bool inspector_view_model::has_selected_entity() const
    {
        return m_selection.has_entity_selection();
    }

    entity inspector_view_model::get_selected_entity() const
    {
        return m_selection.get_first_selected_entity();
    }

    bool inspector_view_model::has_selected_asset() const
    {
        return m_selection.has_asset_selection();
    }

    std::string inspector_view_model::get_selected_asset() const
    {
        return m_selection.get_first_selected_asset();
    }

    std::vector<inspector_component_data> inspector_view_model::get_components() const
    {
        std::vector<inspector_component_data> out;

        if (!has_selected_entity())
            return out;

        entity e = get_selected_entity();

        // UI Registry'deki tüm kayıtlı arayüzleri tara
        for (const auto& [hash, info] : m_world.get_component_registry())
        {
            // Varlık gerçekten bu bileşene sahipse UI verisini hazırla
            if (m_world.has_component_by_hash(e, hash))
            {
                inspector_component_data data;
                data.hash = hash;
                data.ui = &info;
                data.display_name = info.display_name;
                data.default_open = true;
                data.expanded = false;
                data.removable = can_remove(hash);

                out.push_back(std::move(data));
            }
        }

        return out;
    }

    bool inspector_view_model::can_add(size_t hash) const
    {
        if (!has_selected_entity())
            return false;

        return !m_world.has_component_by_hash(get_selected_entity(), hash);
    }

    bool inspector_view_model::can_remove(size_t hash) const
    {
        if (!has_selected_entity())
            return false;

        if (!m_world.has_component_by_hash(get_selected_entity(), hash))
        {
            return false;
        }

        const ComponentUIInfo* info = m_world.get_component_ui(hash);
        if (!info)
            return false;

        std::string name = info->display_name;

        // TODO: (Mimari Not) İsim üzerinden kontrol etmek yerine, ileride bileşen kaydı sırasında 
        // "is_removable" gibi bir bayrak (flag) tanımlanması daha güvenli olacaktır.
        if (name == "Transform" || name == "Name")
            return false;

        return true;
    }

    void inspector_view_model::request_add_component(size_t hash)
    {
        if (!can_add(hash))
            return;

        m_world.add_component_by_hash(get_selected_entity(), hash);
    }

    void inspector_view_model::request_remove_component(size_t hash)
    {
        if (!can_remove(hash))
            return;

        m_world.remove_component_by_hash(get_selected_entity(), hash);
    }

} // namespace vesta