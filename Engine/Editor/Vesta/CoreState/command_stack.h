#pragma once
#include <vector>
#include <memory>
#include "ICommand.h"

namespace vesta {

    class command_stack {
    private:
        std::vector<std::unique_ptr<ICommand>> m_undo_stack;
        std::vector<std::unique_ptr<ICommand>> m_redo_stack;
        IEditableWorld& m_world;

    public:
        command_stack(IEditableWorld& world) : m_world(world) {}

        // Yeni bir komut yürüt ve geçmişe ekle
        void execute_command(std::unique_ptr<ICommand> command) {
            command->execute(m_world);

            // Eğer yeni bir işlem yapıldıysa ileri sarma (redo) geçmişi patlar
            m_redo_stack.clear();

            m_undo_stack.push_back(std::move(command));
            m_world.mark_scene_dirty(); // Her başarılı mutasyon sahneyi kirletir
        }

        void undo() {
            if (m_undo_stack.empty()) return;

            auto command = std::move(m_undo_stack.back());
            m_undo_stack.pop_back();

            command->undo(m_world);
            m_redo_stack.push_back(std::move(command));
            m_world.mark_scene_dirty();
        }

        void redo() {
            if (m_redo_stack.empty()) return;

            auto command = std::move(m_redo_stack.back());
            m_redo_stack.pop_back();

            command->execute(m_world);
            m_undo_stack.push_back(std::move(command));
            m_world.mark_scene_dirty();
        }

        void clear() {
            m_undo_stack.clear();
            m_redo_stack.clear();
        }
    };

} // vesta