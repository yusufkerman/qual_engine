#pragma once
#include "../ICommand.h"
#include <vector>
#include <cstring>

namespace vesta {

    class modify_component_command : public ICommand {
    private:
        entity m_entity;
        size_t m_component_hash;
        size_t m_value_offset;

        std::vector<uint8_t> m_old_value_bytes;
        std::vector<uint8_t> m_new_value_bytes;

    public:
        modify_component_command(entity e, size_t comp_hash, size_t offset,
            const void* old_val, const void* new_val, size_t size)
            : m_entity(e), m_component_hash(comp_hash), m_value_offset(offset)
        {
            m_old_value_bytes.assign((const uint8_t*)old_val, (const uint8_t*)old_val + size);
            m_new_value_bytes.assign((const uint8_t*)new_val, (const uint8_t*)new_val + size);
        }

        virtual void execute(IEditableWorld& world) override {
            void* comp_ptr = world.get_component_raw(m_entity, m_component_hash);
            if (comp_ptr) {
                uint8_t* target_address = (uint8_t*)comp_ptr + m_value_offset;
                std::memcpy(target_address, m_new_value_bytes.data(), m_new_value_bytes.size());
            }
        }

        virtual void undo(IEditableWorld& world) override {
            void* comp_ptr = world.get_component_raw(m_entity, m_component_hash);
            if (comp_ptr) {
                uint8_t* target_address = (uint8_t*)comp_ptr + m_value_offset;
                std::memcpy(target_address, m_old_value_bytes.data(), m_old_value_bytes.size());
            }
        }
    };

} // vesta