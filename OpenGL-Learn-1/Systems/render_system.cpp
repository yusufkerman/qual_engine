#include "render_system.h"
#include "../ECS/registry.h"
#include "../Renderer/MeshRenderer.h"
#include "../ECS/components.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void render_system::render(
    const registry& reg,
    MeshRenderer& renderer,
    Shader& shader,
    const glm::mat4& view,
    const glm::mat4& projection
)
{
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    for (const auto& [e, light] : reg.get_all<light_component>())
    {
        const transform_component* t = reg.get_component<transform_component>(e);
        if (t)
            shader.setVec3("lightPos", t->position);
    }

    for (const auto& [e, mesh] : reg.get_all<mesh_component>())
    {
        const transform_component* transform = reg.get_component<transform_component>(e);
        if (!transform)
            continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, transform->position);
        model = glm::rotate(model, transform->rotation.x, glm::vec3(1, 0, 0));
        model = glm::rotate(model, transform->rotation.y, glm::vec3(0, 1, 0));
        model = glm::rotate(model, transform->rotation.z, glm::vec3(0, 0, 1));
        model = glm::scale(model, transform->scale);

        shader.setMat4("model", model);
        renderer.DrawMesh(mesh.mesh_id);
    }
}