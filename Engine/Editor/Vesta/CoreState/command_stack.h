#pragma once

/**
 * @file command_stack.h
 * @brief Editör komutlarının yürütülmesini, geri alınmasını (undo) ve yinelenmesini (redo) yöneten yığın mekanizması.
 */

#include <vector>
#include <memory>
#include "ICommand.h"

namespace vesta {

    /**
     * @class command_stack
     * @brief ICommand arayüzünden türeyen komutları saklayarak Undo/Redo geçmişini ve sahne kirlilik (dirty) durumunu yönetir.
     */
    class command_stack {
    private:
        std::vector<std::unique_ptr<ICommand>> m_undo_stack;
        std::vector<std::unique_ptr<ICommand>> m_redo_stack;
        IEditableWorld& m_world;

    public:
        /**
         * @brief Komut yığınını dünya referansı ile başlatır.
         * @param world Komutların üzerinde işlem yapacağı düzenlenebilir dünya adaptörü.
         */
        explicit command_stack(IEditableWorld& world) : m_world(world) {}

        /**
         * @brief Yeni bir komutu çalıştırır, geri alma yığınına ekler ve yineleme yığınını sıfırlar.
         * @param command Çalıştırılacak ve sahipliği devralınacak komut nesnesi.
         */
        void execute_command(std::unique_ptr<ICommand> command) {
            if (!command) {
                return;
            }

            command->execute(m_world);

            // Yeni bir mutasyon yapıldığında ileri sarma (redo) zinciri geçersiz kalır
            m_redo_stack.clear();

            m_undo_stack.push_back(std::move(command));
            m_world.mark_scene_dirty();
        }

        /**
         * @brief Son çalıştırılan komutun etkisini geri alır (Undo).
         */
        void undo() {
            if (m_undo_stack.empty()) {
                return;
            }

            auto command = std::move(m_undo_stack.back());
            m_undo_stack.pop_back();

            command->undo(m_world);
            m_redo_stack.push_back(std::move(command));
            m_world.mark_scene_dirty();
        }

        /**
         * @brief Geri alınmış son komutu tekrar çalıştırır (Redo).
         */
        void redo() {
            if (m_redo_stack.empty()) {
                return;
            }

            auto command = std::move(m_redo_stack.back());
            m_redo_stack.pop_back();

            command->execute(m_world);
            m_undo_stack.push_back(std::move(command));
            m_world.mark_scene_dirty();
        }

        /**
         * @brief Geri alınabilecek bir işlemin olup olmadığını sorgular.
         */
        bool can_undo() const {
            return !m_undo_stack.empty();
        }

        /**
         * @brief Yeniden uygulanabilecek bir işlemin olup olmadığını sorgular.
         */
        bool can_redo() const {
            return !m_redo_stack.empty();
        }

        /**
         * @brief Tüm geri alma ve yineleme geçmişini temizler.
         */
        void clear() {
            m_undo_stack.clear();
            m_redo_stack.clear();
        }
    };

} // namespace vesta