#pragma once
#include "../ICommand.h"
#include <string>

namespace vesta {

    class spawn_entity_command : public ICommand {
    private:
        std::string m_name;
        entity m_spawned_entity;
        bool m_has_run = false;

    public:
        spawn_entity_command(const std::string& name = "empty entity")
            : m_name(name), m_spawned_entity(0) {
        }

        virtual void execute(IEditableWorld& world) override {
            if (!m_has_run) {
                m_spawned_entity = world.spawn_entity(m_name);
                m_has_run = true;
            }
            else {
                m_spawned_entity = world.spawn_entity(m_name);
            }
        }

        virtual void undo(IEditableWorld& world) override {
            if (m_has_run && m_spawned_entity != 0) {
                world.destroy_entity(m_spawned_entity);
            }
        }

        entity get_spawned_entity() const { return m_spawned_entity; }
    };

} // vesta