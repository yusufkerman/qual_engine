// =========================================================
// RENDER_SYSTEM.CPP - KESİN ÇÖZÜMÜNÜN OLDUĞU YER
// =========================================================
#include "render_system.h"
#include "../ECS/registry.h"
#include "../Renderer/MeshRenderer.h"
#include "../ECS/components.h"
#include "../Renderer/render_queue.h"
#include "../Renderer/frustum.h"
#include "../shader_manager.h"
#include "../Renderer/material.h"
#include "../Importers/texture_loader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

//  NOT: Parametrelere texture_manager& textureManager eklendi!
void render_system::render(
    const registry& reg,
    const light_batch& lightBatch,
    MeshRenderer& renderer,
    shader_manager& shaderManager,
    asset_manager& assetManager,
    texture_manager& textureManager,
    const glm::mat4& view,
    const glm::mat4& projection
)
{
    glm::mat4 viewProj = projection * view;
    const auto& lights = lightBatch.lights;

    auto items = extract(reg, assetManager, textureManager, viewProj);
    render_queue queue;

    for (auto& item : items)
        queue.batches[item.shader_id].push_back(item);

    for (auto& [shaderID, batch] : queue.batches)
    {
        shader& shader = shaderManager.get(shaderID);
        shader.use();

        shader.setInt("lightCount", (int)lights.size());

        for (int i = 0; i < (int)lights.size() && i < 64; i++)
        {
            const auto& l = lights[i];
            std::string base = "lights[" + std::to_string(i) + "]";
            shader.setInt(base + ".type", l.type);
            shader.setVec3(base + ".position", l.position);
            shader.setVec3(base + ".direction", l.direction);
            shader.setVec3(base + ".color", l.color);
            shader.setFloat(base + ".intensity", l.intensity);
            shader.setFloat(base + ".range", l.range);
            shader.setFloat(base + ".inner_cone", l.inner_cone);
            shader.setFloat(base + ".outer_cone", l.outer_cone);
        }

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        for (auto& item : batch)
        {

            shader.setMat4("model", item.model);
            shader.setVec3("baseColor", item.base_color);

            // 🔥 Evin dokusunun büyüklüğünü ayarlamak için (Tiling / Scale) shader'a gönderiyoruz
            shader.setVec2("u_TextureScale", item.texture_scale);

            // 🖼️ DIFFUSE TEXTURE BAĞLAMA (DÜZELTİLDİ)
            shader.setInt("useTexture", item.diffuse_texture != 0);
            if (item.diffuse_texture != 0)
            {
                // 🔥 KESİN ÇÖZÜM: texture_manager'dan gerçek OpenGL ID'sini istiyoruz!
                unsigned int realGLid = textureManager.get(item.diffuse_texture);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, realGLid); // Gerçek doku bağlandı, morluk bitti!
            }

            // 🗺️ NORMAL MAP BAĞLAMA (DÜZELTİLDİ)
            shader.setInt("useNormalMap", item.normal_texture != 0);
            if (item.normal_texture != 0)
            {
                // 🔥 KESİN ÇÖZÜM: Normal map için de gerçek ID'yi çekiyoruz
                unsigned int realNormalGLid = textureManager.get(item.normal_texture);

                shader.setFloat("normalStrength", item.normal_strength);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, realNormalGLid);
            }


            renderer.Draw(item);
        }
    }
}

std::vector<render_item> render_system::extract(
    const registry& reg,
    asset_manager& assetManager,
    class texture_manager& textureManager,
    const glm::mat4& viewProj
)
{
    std::vector<render_item> items;
    frustum f = buildFrustum(viewProj);
    constexpr float kCullBias = 0.01f;

    for (const auto& [e, mesh] : reg.get_all<mesh_component>())
    {
        const auto* transform = reg.get_component<transform_component>(e);
        const material_component* matComp = reg.get_component<material_component>(e);
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

        float maxScale = glm::max(transform->scale.x, glm::max(transform->scale.y, transform->scale.z));
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

        if (!inside) continue;

        render_item item;
        item.model = model;
        item.mesh_id = mesh.mesh_id;

        if (matComp)
        {
            const material* mat = assetManager.try_get<material>(matComp->material_id);

            if (mat)
            {
                item.shader_id = mat->shader_id;
                item.base_color = mat->base_color;
                item.normal_strength = mat->normal_strength;
                item.texture_scale = mat->texture_scale;

                // 🖼️ DIFFUSE TEXTURE FİLTRESİ
                // ID sıfır değilse VE gerçekten textureManager içinde bu kayıt yüklenmişse kabul et
                if (mat->diffuse_texture != 0 && textureManager.has(mat->diffuse_texture))
                {
                    item.diffuse_texture = mat->diffuse_texture;
                }
                else
                {
                    item.diffuse_texture = 0; // Çöp ID'yi temizle, hata basılmasını engelle!
                }

                // 🗺️ NORMAL MAP FİLTRESİ
                if (mat->normal_texture != 0 && textureManager.has(mat->normal_texture))
                {
                    item.normal_texture = mat->normal_texture;
                }
                else
                {
                    item.normal_texture = 0; // Çöp ID ise 0 yap, düz geç!
                }
            }
            else
            {
                item.shader_id = 0;
                item.base_color = glm::vec3(1.0f, 0.0f, 1.0f); // Fallback rengi mor kalabilir
                item.diffuse_texture = 0;
                item.normal_texture = 0;
                item.texture_scale = glm::vec2(1.0f);
            }
        }
        else
        {
            item.shader_id = 0;
            item.base_color = glm::vec3(1.0f);
            item.diffuse_texture = 0;
            item.normal_texture = 0;
            item.texture_scale = glm::vec2(1.0f);
        }

        items.push_back(item);
    }

    return items;
}