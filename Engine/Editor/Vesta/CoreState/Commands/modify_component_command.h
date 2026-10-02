#pragma once

/**
 * @file modify_component_command.h
 * @brief ECS bileşenleri üzerindeki alan değişikliklerini bayt seviyesinde geri alınabilir kılan komut sınıfı.
 */

#include "../ICommand.h"
#include <vector>
#include <cstring>
#include <cstdint>

namespace vesta {

    /**
     * @class modify_component_command
     * @brief Tip-silinmiş (type-erased) bellek ofseti ve bayt tamponları aracılığıyla bileşen alan değişikliklerini yönetir.
     */
    class modify_component_command : public ICommand {
    private:
        entity m_entity;
        size_t m_component_hash;
        size_t m_value_offset;

        std::vector<uint8_t> m_old_value_bytes;
        std::vector<uint8_t> m_new_value_bytes;

    public:
        /**
         * @brief Bileşenin değişen bayt aralığını kaydederek komutu başlatır.
         *
         * @param e Değişikliğin yapıldığı varlık.
         * @param comp_hash İlgili bileşenin tip kimliği.
         * @param offset Bileşen yapısı içerisindeki hedef alanın ofseti (bayt cinsinden).
         * @param old_val Alanın eski değerine işaretçi.
         * @param new_val Alanın yeni değerine işaretçi.
         * @param size Değiştirilen alanın bayt boyutu.
         */
        modify_component_command(entity e, size_t comp_hash, size_t offset,
            const void* old_val, const void* new_val, size_t size)
            : m_entity(e), m_component_hash(comp_hash), m_value_offset(offset)
        {
            const auto* old_bytes = static_cast<const uint8_t*>(old_val);
            const auto* new_bytes = static_cast<const uint8_t*>(new_val);

            m_old_value_bytes.assign(old_bytes, old_bytes + size);
            m_new_value_bytes.assign(new_bytes, new_bytes + size);
        }

        void execute(IEditableWorld& world) override {
            void* comp_ptr = world.get_component_raw(m_entity, m_component_hash);
            if (comp_ptr) {
                auto* target_address = static_cast<uint8_t*>(comp_ptr) + m_value_offset;
                std::memcpy(target_address, m_new_value_bytes.data(), m_new_value_bytes.size());
            }
        }

        void undo(IEditableWorld& world) override {
            void* comp_ptr = world.get_component_raw(m_entity, m_component_hash);
            if (comp_ptr) {
                auto* target_address = static_cast<uint8_t*>(comp_ptr) + m_value_offset;
                std::memcpy(target_address, m_old_value_bytes.data(), m_old_value_bytes.size());
            }
        }
    };

} // namespace vesta