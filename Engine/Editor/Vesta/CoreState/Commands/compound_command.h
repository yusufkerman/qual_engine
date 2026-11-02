#pragma once

/**
 * @file compound_command.h
 * @brief Birden fazla komutu tek bir geri alınabilir (Undo/Redo) işlem olarak gruplayan komut sınıfı.
 */

#include "../ICommand.h"
#include <vector>
#include <memory>

namespace vesta {

    /**
     * @class compound_command
     * @brief Birden fazla eylemi tek bir atomik komut altında toplar.
     */
    class compound_command : public ICommand {
    private:
        std::vector<std::unique_ptr<ICommand>> m_commands;

    public:
        compound_command() = default;
        explicit compound_command(std::vector<std::unique_ptr<ICommand>> commands)
            : m_commands(std::move(commands)) {}

        void add_command(std::unique_ptr<ICommand> cmd) {
            if (cmd) {
                m_commands.push_back(std::move(cmd));
            }
        }

        void execute(IEditableWorld& world) override {
            for (auto& cmd : m_commands) {
                if (cmd) {
                    cmd->execute(world);
                }
            }
        }

        void undo(IEditableWorld& world) override {
            for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
                if (*it) {
                    (*it)->undo(world);
                }
            }
        }

        [[nodiscard]] bool empty() const noexcept { return m_commands.empty(); }
        [[nodiscard]] size_t size() const noexcept { return m_commands.size(); }
    };

} // namespace vesta

