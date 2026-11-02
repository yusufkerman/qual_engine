#pragma once

/**
 * @file rename_entity_command.h
 * @brief Varlık yeniden adlandırma (rename) eylemini ve geri alma sürecini yönetir.
 */

#include "../ICommand.h"
#include "../../../ECS/registry.h"
#include "../../../ECS/components.h"
#include <string>

namespace vesta {

    /**
     * @class rename_entity_command
     * @brief Varlık isim değişikliğini geri alınabilir (Undo/Redo) komut olarak saklar.
     */
    class rename_entity_command : public ICommand {
    private:
        entity m_entity{ 0 };
        std::string m_old_name;
        std::string m_new_name;

    public:
        rename_entity_command(entity e, std::string old_name, std::string new_name)
            : m_entity(e), m_old_name(std::move(old_name)), m_new_name(std::move(new_name))
        {
        }

        void execute(IEditableWorld& world) override {
            if (auto* reg = world.get_registry()) {
                if (auto* n = reg->get_component<name_component>(m_entity)) {
                    n->name = m_new_name;
                    world.mark_scene_dirty();
                }
            }
        }

        void undo(IEditableWorld& world) override {
            if (auto* reg = world.get_registry()) {
                if (auto* n = reg->get_component<name_component>(m_entity)) {
                    n->name = m_old_name;
                    world.mark_scene_dirty();
                }
            }
        }
    };

} // namespace vesta

