/**
 * @file transform_wrapper.cpp
 * @brief Transform ergonomik sarmalayıcı uygulaması.
 */

#include "transform_wrapper.h"
#include "../ECS/components.h"
#include "../ECS/registry.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace gameplay
{
    transform_wrapper::transform_wrapper(entity e, registry* reg)
        : m_entity(e), m_reg(reg)
    {
    }

    bool transform_wrapper::is_valid() const
    {
        return m_reg != nullptr && m_entity != 0 && m_reg->has_component<transform_component>(m_entity);
    }

    transform_component* transform_wrapper::raw()
    {
        if (!m_reg || m_entity == 0)
        {
            return nullptr;
        }
        return m_reg->get_component<transform_component>(m_entity);
    }

    glm::vec3 transform_wrapper::get_position() const
    {
        if (!m_reg || m_entity == 0)
        {
            return glm::vec3{ 0.0f };
        }
        auto* t = m_reg->get_component<transform_component>(m_entity);
        return t ? t->position : glm::vec3{ 0.0f };
    }

    void transform_wrapper::set_position(const glm::vec3& pos)
    {
        auto* t = raw();
        if (t)
        {
            t->position = pos;
        }
    }

    glm::vec3 transform_wrapper::get_rotation_euler() const
    {
        if (!m_reg || m_entity == 0)
        {
            return glm::vec3{ 0.0f };
        }
        auto* t = m_reg->get_component<transform_component>(m_entity);
        return t ? t->rotation : glm::vec3{ 0.0f };
    }

    void transform_wrapper::set_rotation_euler(const glm::vec3& euler_rad)
    {
        auto* t = raw();
        if (t)
        {
            t->rotation = euler_rad;
        }
    }

    glm::vec3 transform_wrapper::get_scale() const
    {
        if (!m_reg || m_entity == 0)
        {
            return glm::vec3{ 1.0f };
        }
        auto* t = m_reg->get_component<transform_component>(m_entity);
        return t ? t->scale : glm::vec3{ 1.0f };
    }

    void transform_wrapper::set_scale(const glm::vec3& scale)
    {
        auto* t = raw();
        if (t)
        {
            t->scale = scale;
        }
    }

    void transform_wrapper::translate(const glm::vec3& delta)
    {
        auto* t = raw();
        if (t)
        {
            t->position += delta;
        }
    }

    void transform_wrapper::rotate_euler(const glm::vec3& delta_euler_rad)
    {
        auto* t = raw();
        if (t)
        {
            t->rotation += delta_euler_rad;
        }
    }

    glm::vec3 transform_wrapper::forward() const
    {
        const glm::vec3 rot = get_rotation_euler();
        glm::mat4 rotMatrix(1.0f);
        rotMatrix = glm::rotate(rotMatrix, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        rotMatrix = glm::rotate(rotMatrix, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rotMatrix = glm::rotate(rotMatrix, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

        // OpenGL sağ el kuralı: -Z ekseni ileri yöndür
        return glm::normalize(-glm::vec3(rotMatrix[2]));
    }

    glm::vec3 transform_wrapper::right() const
    {
        const glm::vec3 rot = get_rotation_euler();
        glm::mat4 rotMatrix(1.0f);
        rotMatrix = glm::rotate(rotMatrix, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        rotMatrix = glm::rotate(rotMatrix, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rotMatrix = glm::rotate(rotMatrix, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

        // +X ekseni sağ yöndür
        return glm::normalize(glm::vec3(rotMatrix[0]));
    }

    glm::vec3 transform_wrapper::up() const
    {
        const glm::vec3 rot = get_rotation_euler();
        glm::mat4 rotMatrix(1.0f);
        rotMatrix = glm::rotate(rotMatrix, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        rotMatrix = glm::rotate(rotMatrix, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rotMatrix = glm::rotate(rotMatrix, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

        // +Y ekseni yukarı yöndür
        return glm::normalize(glm::vec3(rotMatrix[1]));
    }

    void transform_wrapper::look_at(const glm::vec3& target_point, const glm::vec3& world_up)
    {
        auto* t = raw();
        if (!t)
        {
            return;
        }

        const glm::vec3 dir = target_point - t->position;
        if (glm::length(dir) < 0.0001f)
        {
            return;
        }

        const glm::vec3 forward_dir = glm::normalize(dir);
        const float pitch = -std::asin(std::clamp(forward_dir.y, -1.0f, 1.0f));
        const float yaw = std::atan2(forward_dir.x, -forward_dir.z);

        t->rotation = glm::vec3(pitch, yaw, 0.0f);
    }
}

