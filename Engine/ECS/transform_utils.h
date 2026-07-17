#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "entity.h"
#include "registry.h"
#include "components.h"   
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ecs_transform {

    inline glm::mat4 compute_local_matrix(const transform_component* t)
    {
        glm::mat4 model(1.0f);
        model = glm::translate(model, t->position);
        model = glm::rotate(model, t->rotation.x, glm::vec3(1, 0, 0));
        model = glm::rotate(model, t->rotation.y, glm::vec3(0, 1, 0));
        model = glm::rotate(model, t->rotation.z, glm::vec3(0, 0, 1));
        model = glm::scale(model, t->scale);
        return model;
    }

    inline glm::mat4 get_world_matrix(entity e, const registry& reg)
    {
        const auto* transform = reg.get_component<transform_component>(e);
        if (!transform)
            return glm::mat4(1.0f);

        glm::mat4 local = compute_local_matrix(transform);

        const auto* hierarchy = reg.get_component<hierarchy_component>(e);
        if (hierarchy && hierarchy->parent != 0)
        {
            return get_world_matrix(hierarchy->parent, reg) * local;
        }

        return local;
    }

    inline void apply_world_matrix_as_local(transform_component& t, const glm::mat4& world)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat rotation_quat;
        glm::vec3 translation, scale;

        if (!glm::decompose(world, scale, rotation_quat, translation, skew, perspective))
            return; // decompose başarısızsa mevcut değerlere dokunma

        t.position = translation;
        t.scale = scale;

        // 🎯 glm::eulerAngles() KULLANMIYORUZ çünkü onun konvansiyonu,
        // compute_local_matrix()'teki Rx*Ry*Rz sırasıyla birebir uyuşmuyor,
        // bu da reparent sonrası küçük görsel rotasyon sapmalarına yol açıyordu.
        // Bunun yerine rotation matrix'ten AYNI sıraya (Rx*Ry*Rz) uygun elle extraction yapıyoruz.
        glm::mat3 R = glm::mat3_cast(rotation_quat);

        float pitch, yaw, roll; // x, y, z

        // 🎯 DÜZELTME: GLM mat3 column-major saklanıyor, yani m[col][row].
        // Matematiksel R(row,col) değerine erişmek için R[col][row] kullanılmalı.
        // Bir önceki versiyonda bu ters kullanılmıştı, hafif rotasyon sapmasının
        // gerçek sebebi buydu.
        float sinYaw = glm::clamp(R[2][0], -1.0f, 1.0f); // math R[0][2]

        if (sinYaw < 1.0f) {
            if (sinYaw > -1.0f) {
                pitch = std::atan2(-R[2][1], R[2][2]); // math -R[1][2], R[2][2]
                yaw = std::asin(sinYaw);
                roll = std::atan2(-R[1][0], R[0][0]); // math -R[0][1], R[0][0]
            }
            else {
                // Gimbal lock: sinYaw == -1
                pitch = 0.0f;
                yaw = -glm::half_pi<float>();
                roll = -std::atan2(R[0][1], R[1][1]); // math R[1][0], R[1][1]
            }
        }
        else {
            // Gimbal lock: sinYaw == 1
            pitch = 0.0f;
            yaw = glm::half_pi<float>();
            roll = std::atan2(R[0][1], R[1][1]);
        }

        t.rotation = glm::vec3(pitch, yaw, roll);
    }

} // namespace ecs_transform