/**
 * @file light_system.cpp
 * @brief Işık bileşenlerinin dönüştürülmesi ve yön vektörü hesaplama uygulaması.
 */

#include "light_system.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {

    /**
     * @brief Euler dönüş açılarından (radyan) ileri yön vektörünü (Forward) hesaplar.
     * @param rot X, Y, Z eksenlerindeki rotasyon değerleri.
     * @return Birim ileri yön vektörü.
     */
    glm::vec3 get_forward_from_rotation(const glm::vec3& rot)
    {
        glm::mat4 m = glm::mat4(1.0f);

        m = glm::rotate(m, rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

        const glm::vec3 forward = glm::vec3(m * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

        return glm::normalize(forward);
    }

} // anonymous namespace

std::vector<light_data> light_system::extract(const registry& reg)
{
    const auto& lights = reg.get_all<light_component>();
    std::vector<light_data> result;
    result.reserve(lights.size());

    for (const auto& [e, light] : lights)
    {
        const auto* transform = reg.get_component<transform_component>(e);
        if (!transform)
        {
            continue;
        }

        light_data data{};

        data.color = light.color;
        data.intensity = light.intensity;
        data.range = light.range;
        data.inner_cone = light.inner_cone;
        data.outer_cone = light.outer_cone;

        data.position = transform->position;
        data.direction = get_forward_from_rotation(transform->rotation);

        if (light.type == light_type::directional)
        {
            data.type = 0;
        }
        else if (light.type == light_type::point)
        {
            data.type = 1;
        }
        else
        {
            data.type = 2;
        }

        result.push_back(data);
    }

    return result;
}