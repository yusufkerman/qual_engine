#include "render_system.h"

#include "../ECS/registry.h"
#include "../Renderer/MeshRenderer.h"
#include "../ECS/components.h"
#include "../Renderer/render_queue.h"
#include "../Renderer/frustum.h"
#include "../shader_manager.h"
#include "../Renderer/material_manager.h"
#include "../Renderer/material.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// =========================
// RENDER
// =========================
void render_system::render(
    const registry& reg,
    MeshRenderer& renderer,
    shader_manager& shaderManager,
    const material_manager& materialManager,
    const glm::mat4& view,
    const glm::mat4& projection
)
{
    glm::mat4 viewProj = projection * view;

    auto items = extract(reg, materialManager, viewProj);

    render_queue queue;

    for (auto& item : items)
        queue.batches[item.shader_id].push_back(item);

    for (auto& [shaderID, batch] : queue.batches)
    {
        shader& shader = shaderManager.get(shaderID);
        shader.use();

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        for (auto& item : batch)
        {
            shader.setMat4("model", item.model);
            shader.setVec3("color", item.base_color);

            if (item.diffuse_texture != 0)
                glBindTexture(GL_TEXTURE_2D, item.diffuse_texture);

            renderer.Draw(item);
        }
    }
}

// =========================
// EXTRACT
// =========================
std::vector<render_item> render_system::extract(
    const registry& reg,
    const material_manager& materialManager,
    const glm::mat4& viewProj
)
{
    std::vector<render_item> items;

    frustum f = buildFrustum(viewProj);

    constexpr float kCullBias = 0.01f;

    for (const auto& [e, mesh] : reg.get_all<mesh_component>())
    {
        const auto* transform = reg.get_component<transform_component>(e);
        const material_component* matComp =
            reg.get_component<material_component>(e);
        const auto* bounds = reg.get_component<bounds_component>(e);

        if (!transform) continue;

        glm::mat4 model(1.0f);
        model = glm::translate(model, transform->position);
        model = glm::rotate(model, transform->rotation.x, glm::vec3(1, 0, 0));
        model = glm::rotate(model, transform->rotation.y, glm::vec3(0, 1, 0));
        model = glm::rotate(model, transform->rotation.z, glm::vec3(0, 0, 1));
        model = glm::scale(model, transform->scale);

        glm::vec3 localCenter = bounds ? bounds->localCenter : glm::vec3(0.0f);
        glm::vec3 worldCenter = glm::vec3(model * glm::vec4(localCenter, 1.0f));

        float maxScale = glm::max(transform->scale.x,
            glm::max(transform->scale.y, transform->scale.z));

        float baseRadius = bounds ? bounds->localRadius : transform->boundingRadius;
        float radius = baseRadius * maxScale + kCullBias;

        bool inside = true;

        for (const auto& plane : f.planes)
        {
            if (plane.distance(worldCenter) < -radius)
            {
                inside = false;
                break;
            }
        }

        if (!inside)
            continue;

        render_item item;

        item.model = model;
        item.mesh_id = mesh.mesh_id;

        if (matComp)
        {
            const material& mat = materialManager.get(matComp->material_id);

            item.shader_id = mat.shader_id;
            item.base_color = mat.base_color;
            item.diffuse_texture = mat.diffuse_texture;
        }
        else
        {
            item.shader_id = 0;
            item.base_color = glm::vec3(1.0f);
            item.diffuse_texture = 0;
        }

        items.push_back(item);
    }

    return items;
}