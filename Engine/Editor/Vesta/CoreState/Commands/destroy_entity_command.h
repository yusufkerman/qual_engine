#pragma once

/**
 * @file destroy_entity_command.h
 * @brief Sahneden bir varlığı yok etme (destroy) eylemini ve alt hiyerarşisiyle birlikte geri alma sürecini sarmalayan komut sınıfı.
 */

#include "../ICommand.h"
#include "../../../ECS/registry.h"
#include "../../../ECS/components.h"
#include <vector>
#include <unordered_set>

namespace vesta {

    namespace detail {
        inline void collect_descendants_for_backup(const registry& reg, entity parent, std::vector<entity>& out) {
            std::unordered_set<entity> visited;
            const auto* hc = reg.get_component<hierarchy_component>(parent);
            if (!hc) return;

            entity current = hc->first_child;
            int safety = 0;
            while (current != 0 && safety++ < 100000) {
                if (visited.contains(current)) break;
                visited.insert(current);
                if (reg.valid(current)) {
                    out.push_back(current);
                    collect_descendants_for_backup(reg, current, out);
                }
                const auto* child_hc = reg.get_component<hierarchy_component>(current);
                current = child_hc ? child_hc->next_sibling : 0;
            }
        }
    }

    /**
     * @class destroy_entity_command
     * @brief Hedef varlığın ve tüm alt çocuklarının silinmesini ve geri alma (undo) anında tüm bileşenleriyle geri yüklenmesini yönetir.
     */
    class destroy_entity_command : public ICommand {
    private:
        entity m_target_entity{ 0 };
        entity m_saved_parent{ 0 };
        std::vector<entity> m_subtree_entities;
        registry m_backup_registry;
        bool m_has_backup = false;

        void perform_backup(const registry& reg) {
            if (!reg.valid(m_target_entity)) return;

            m_subtree_entities.clear();
            m_subtree_entities.push_back(m_target_entity);
            detail::collect_descendants_for_backup(reg, m_target_entity, m_subtree_entities);

            if (const auto* hc = reg.get_component<hierarchy_component>(m_target_entity)) {
                m_saved_parent = hc->parent;
            } else {
                m_saved_parent = 0;
            }

            for (entity e : m_subtree_entities) {
                if (reg.valid(e)) {
                    reg.clone_entity(e, m_backup_registry);
                }
            }
            m_has_backup = true;
        }

    public:
        explicit destroy_entity_command(entity e)
            : m_target_entity(e)
        {
        }

        destroy_entity_command(entity e, const registry& source_reg)
            : m_target_entity(e)
        {
            perform_backup(source_reg);
        }

        void execute(IEditableWorld& world) override {
            if (!m_has_backup) {
                if (auto* reg = world.get_registry()) {
                    perform_backup(*reg);
                }
            }
            world.destroy_entity(m_target_entity);
        }

        void undo(IEditableWorld& world) override {
            auto* reg = world.get_registry();
            if (!reg || !m_has_backup) return;

            for (entity e : m_subtree_entities) {
                if (m_backup_registry.valid(e)) {
                    m_backup_registry.clone_entity(e, *reg);
                }
            }

            if (m_saved_parent != 0 && reg->valid(m_saved_parent)) {
                world.set_entity_parent(m_target_entity, m_saved_parent);
            }

            world.mark_scene_dirty();
        }

        [[nodiscard]] entity get_target_entity() const noexcept { return m_target_entity; }
    };

} // namespace vesta
