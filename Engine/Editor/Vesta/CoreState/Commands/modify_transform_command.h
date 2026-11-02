#pragma once

/**
 * @file modify_transform_command.h
 * @brief Varlıkların 3B/2B dönüşüm (Position, Rotation, Scale) değişikliklerini Undo/Redo destekli yöneten komut sınıfı.
 */

#include "../ICommand.h"
#include <glm/glm.hpp>
#include <vector>
#include <typeinfo>
#include "../../../ECS/components.h"

namespace vesta {

    /**
     * @struct entity_transform_record
     * @brief Tek bir varlığın dönüşüm değişikliği öncesi ve sonrası durumlarını tutan kayıt.
     */
    struct entity_transform_record {
        entity ent = 0;
        glm::vec3 old_position{ 0.0f };
        glm::vec3 old_rotation{ 0.0f };
        glm::vec3 old_scale{ 1.0f };

        glm::vec3 new_position{ 0.0f };
        glm::vec3 new_rotation{ 0.0f };
        glm::vec3 new_scale{ 1.0f };
    };

    /**
     * @class modify_transform_command
     * @brief Gizmo veya Inspector üzerinden değiştirilen varlıkların dönüşüm durumlarını saklayarak geri alma/yineleme sağlar.
     */
    class modify_transform_command : public ICommand {
    private:
        std::vector<entity_transform_record> m_records;

    public:
        explicit modify_transform_command(std::vector<entity_transform_record> records)
            : m_records(std::move(records)) {}

        modify_transform_command(entity e,
            const glm::vec3& oldPos, const glm::vec3& oldRot, const glm::vec3& oldScale,
            const glm::vec3& newPos, const glm::vec3& newRot, const glm::vec3& newScale)
        {
            m_records.push_back({ e, oldPos, oldRot, oldScale, newPos, newRot, newScale });
        }

        void execute(IEditableWorld& world) override {
            for (const auto& rec : m_records) {
                auto* comp = static_cast<transform_component*>(
                    world.get_component_raw(rec.ent, typeid(transform_component).hash_code()));
                if (comp) {
                    comp->position = rec.new_position;
                    comp->rotation = rec.new_rotation;
                    comp->scale = rec.new_scale;
                }
            }
        }

        void undo(IEditableWorld& world) override {
            for (const auto& rec : m_records) {
                auto* comp = static_cast<transform_component*>(
                    world.get_component_raw(rec.ent, typeid(transform_component).hash_code()));
                if (comp) {
                    comp->position = rec.old_position;
                    comp->rotation = rec.old_rotation;
                    comp->scale = rec.old_scale;
                }
            }
        }

        [[nodiscard]] const std::vector<entity_transform_record>& get_records() const noexcept {
            return m_records;
        }
    };

} // namespace vesta

