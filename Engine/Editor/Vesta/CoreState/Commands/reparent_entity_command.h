#pragma once

/**
 * @file reparent_entity_command.h
 * @brief Bir veya birden fazla varlığın ebeveyn atama (reparenting) eylemini ve geri alma sürecini yönetir.
 */

#include "../ICommand.h"
#include "../../../ECS/registry.h"
#include "../../../ECS/components.h"
#include <vector>

namespace vesta {

    /**
     * @struct reparent_entry
     * @brief Reparent işlemine katılan bir varlığın eski ve yeni durum bilgilerini tutar.
     */
    struct reparent_entry {
        entity child{ 0 };
        entity old_parent{ 0 };
        transform_component old_transform{};
        bool has_transform{ false };
    };

    /**
     * @class reparent_entity_command
     * @brief Çoklu veya tekil varlık ebeveyn değiştirme eylemlerini geri alınabilir (Undo/Redo) hale getirir.
     */
    class reparent_entity_command : public ICommand {
    private:
        std::vector<reparent_entry> m_entries;
        entity m_target_parent{ 0 };

    public:
        reparent_entity_command(const std::vector<entity>& children, entity target_parent, const registry& reg)
            : m_target_parent(target_parent)
        {
            m_entries.reserve(children.size());
            for (entity child : children) {
                if (child == 0 || !reg.valid(child)) continue;

                reparent_entry entry;
                entry.child = child;
                if (const auto* hc = reg.get_component<hierarchy_component>(child)) {
                    entry.old_parent = hc->parent;
                }
                if (const auto* tc = reg.get_component<transform_component>(child)) {
                    entry.old_transform = *tc;
                    entry.has_transform = true;
                }
                m_entries.push_back(entry);
            }
        }

        reparent_entity_command(entity child, entity target_parent, const registry& reg)
            : reparent_entity_command(std::vector<entity>{ child }, target_parent, reg)
        {
        }

        void execute(IEditableWorld& world) override {
            for (const auto& entry : m_entries) {
                world.set_entity_parent(entry.child, m_target_parent);
            }
            world.mark_scene_dirty();
        }

        void undo(IEditableWorld& world) override {
            for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it) {
                world.set_entity_parent(it->child, it->old_parent);
                if (it->has_transform) {
                    if (auto* reg = world.get_registry()) {
                        if (auto* tc = reg->get_component<transform_component>(it->child)) {
                            *tc = it->old_transform;
                        }
                    }
                }
            }
            world.mark_scene_dirty();
        }

        [[nodiscard]] bool empty() const noexcept { return m_entries.empty(); }
    };

} // namespace vesta

