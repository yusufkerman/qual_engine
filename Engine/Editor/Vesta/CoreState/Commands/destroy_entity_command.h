#pragma once
#include "../ICommand.h"
#include <iostream>

namespace vesta {

    class destroy_entity_command : public ICommand {
    private:
        entity m_target_entity;

    public:
        destroy_entity_command(entity e) : m_target_entity(e) {}

        virtual void execute(IEditableWorld& world) override {
            world.destroy_entity(m_target_entity);
        }

        virtual void undo(IEditableWorld& world) override {
            std::cout << "[vesta l2] undo: destroy_entity_command tetiklendi (sera ile entegre edilecek)\n";
        }
    };

} // vesta