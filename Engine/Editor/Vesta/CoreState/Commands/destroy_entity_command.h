#pragma once

/**
 * @file destroy_entity_command.h
 * @brief Sahneden bir varlığı yok etme (destroy) eylemini ve geri alma sürecini sarmalayan komut sınıfı.
 */

#include "../ICommand.h"
#include <iostream>

namespace vesta {

    /**
     * @class destroy_entity_command
     * @brief Hedef varlığın sahneden silinmesini yönetir.
     */
    class destroy_entity_command : public ICommand {
    private:
        entity m_target_entity;

    public:
        /**
         * @brief Yok edilecek varlık kimliği ile komutu başlatır.
         * @param e Silinecek varlığın kimliği.
         */
        explicit destroy_entity_command(entity e) : m_target_entity(e) {}

        void execute(IEditableWorld& world) override {
            world.destroy_entity(m_target_entity);
        }

        void undo(IEditableWorld& world) override {
            // Not: Sahne serileştirme / varlık kurtarma katmanı bağlandığında restore işlemi yürütülecektir.
            std::cout << "[Vesta Command] destroy_entity_command geri alma (undo) tetiklendi.\n";
        }
    };

} // namespace vesta