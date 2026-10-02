#pragma once

/**
 * @file spawn_entity_command.h
 * @brief Sahneye yeni varlık oluşturma (spawn) ve geri alma (undo) işlemlerini yöneten komut sınıfı.
 */

#include "../ICommand.h"
#include <string>

namespace vesta {

    /**
     * @class spawn_entity_command
     * @brief Varlık oluşturma işlemini kaydeder ve geri alma durumunda oluşturulan varlığı sahneden kaldırır.
     */
    class spawn_entity_command : public ICommand {
    private:
        std::string m_name;
        entity m_spawned_entity;
        bool m_has_run = false;

    public:
        /**
         * @brief Oluşturulacak varsayılan varlık adı ile komutu başlatır.
         * @param name Oluşturulacak varlığın adı.
         */
        explicit spawn_entity_command(const std::string& name = "empty entity")
            : m_name(name), m_spawned_entity(0) {
        }

        void execute(IEditableWorld& world) override {
            m_spawned_entity = world.spawn_entity(m_name);
            m_has_run = true;
        }

        void undo(IEditableWorld& world) override {
            if (m_has_run && m_spawned_entity != 0) {
                world.destroy_entity(m_spawned_entity);
            }
        }

        /**
         * @brief Komut çalıştırıldıktan sonra üretilen varlığın kimliğini döndürür.
         */
        entity get_spawned_entity() const { return m_spawned_entity; }
    };

} // namespace vesta