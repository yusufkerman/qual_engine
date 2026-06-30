#include "render_system.h"
#include "../Engine/engine_context.h" 
#include "../ECS/registry.h"
#include "../ECS/components.h"
#include "../Renderer/frustum.h"
#include "../Renderer/material.h"
#include "../shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm> // std::min için

// MeshRenderer enum/sabitleri (mesh id vb.)
#include "../Renderer/MeshRenderer.h"

// Constructor artık tamamen temiz ve güvenli
render_system::render_system()
    : m_LightUBO(0)
{
}

/** * @brief Render sisteminin GPU kaynaklarını güvenli bir şekilde oluşturur.
 * Bu fonksiyon GLAD ilklendirilmeden önce ÇAĞRILMAMALIDIR.
 */
void render_system::init()
{
    if (m_LightUBO != 0) return; // Zaten oluşturulduysa mükerrer işlem yapma

    size_t uboSize = 16 + (sizeof(light_data) * MAX_LIGHT_COUNT);

    // Not: UBO sistemin kendi iç yönetimi olduğu için buradaki ham çağrılar kalmıştır.
    glGenBuffers(1, &m_LightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_LightUBO);
    glBufferData(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_LightUBO);

    std::cout << "[RenderSystem] Light UBO successfully initialized." << std::endl;
}

void render_system::bind_light_buffer(shader& s)
{
    unsigned int blockIndex = glGetUniformBlockIndex(s.ID, "LightBuffer");
    if (blockIndex != GL_INVALID_INDEX)
        glUniformBlockBinding(s.ID, blockIndex, 0);
}

/**
 * @brief GPU kaynaklarını temizler.
 */
render_system::~render_system()
{
    if (m_LightUBO != 0)
    {
        glDeleteBuffers(1, &m_LightUBO);
    }
}

/**
 * @brief Sahneyi render eder (ana render pipeline).
 *
 * Pipeline sırası:
 * 1. Işık verisi GPU'ya gönderilir (UBO)
 * 2. ECS'den render edilebilir objeler çıkarılır (extract)
 * 3. Shader bazlı batching yapılır
 * 4. Draw call'lar gerçekleştirilir
 */
void render_system::render(
    const registry& reg,
    const light_batch& lightBatch,
    EngineContext& ctx,
    const glm::mat4& view,
    const glm::mat4& projection
)
{
    // Mevcut bağlı olan FBO ve Viewport ayarlarını yedekle
    GLint previousFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glm::mat4 viewProj = projection * view;

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosition = glm::vec3(invView[3]);

    // Scene içindeki ışık listesi alınır
    const auto& lights = lightBatch.lights;

    int lightCount = (int)lights.size();
    if (lightCount > MAX_LIGHT_COUNT)
        lightCount = MAX_LIGHT_COUNT;

    // Işık sayısı GPU'ya yazılır
    glBindBuffer(GL_UNIFORM_BUFFER, m_LightUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &lightCount);

    // Işık verileri GPU'ya yazılır
    if (lightCount > 0)
    {
        glBufferSubData(GL_UNIFORM_BUFFER, 16,
            lightCount * sizeof(light_data),
            lights.data());
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Frame başı render cache temizlenir
    m_RenderItemsCache.clear();

    // ECS → render item extraction
    extract(reg, ctx, viewProj);

    // Shader bazlı batch map temizlenir
    m_RenderQueueCache.batches.clear();

    // Render item'lar shader ID'ye göre gruplanır
    for (const auto& item : m_RenderItemsCache)
        m_RenderQueueCache.batches[item.shader_id].push_back(item);

    // ------------------------------------------------------------------------
    // PASS 1: SHADOW MAP RENDER (Işığın Gözünden Derinlik Çizimi)
    // ------------------------------------------------------------------------
    glm::mat4 lightSpaceMatrix(1.0f);
    bool hasDirectionalLight = false;

    // Sahnedeki ilk yönlü ışığı (Type == 0) bulup matrisini hesaplıyoruz
    for (int i = 0; i < lightCount; ++i)
    {
        if (lights[i].type == 0) // Directional Light
        {
            lightSpaceMatrix = calculate_light_space_matrix(lights[i], cameraPosition);
            hasDirectionalLight = true;
            break;
        }
    }

    if (hasDirectionalLight)
    {
        // Gölge tuvalini bağla (glViewport otomatik 2048x2048 olur)
        ctx.shadowFBO.bind();

        // Gölge shader'ını çağır
        shader& shadowShader = ctx.shaderManager.get(ctx.shadowShaderID);
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        // Frustum culling'e takılmadan, sahnedeki tüm nesneleri ışık gözünden çiziyoruz
        for (auto& [shaderID, batch] : m_RenderQueueCache.batches)
        {
            for (const auto& item : batch)
            {
                shadowShader.setMat4("model", item.model);
                ctx.renderer.Draw(item, ctx.textureManager);
            }
        }

        ctx.renderer.BindFramebuffer(previousFBO);
    }

    // ------------------------------------------------------------------------
    // PASS 2: ANA RENDER PASS (Normal Sahne Çizimi)
    // ------------------------------------------------------------------------
    // ÖNEMLİ: Gölge pass'i viewport'u 2048x2048 yapmıştı. 
    // Ana ekrana dönerken kendi ekran boyutlarımıza geri set etmeliyiz.
    ctx.renderer.SetViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    // Her shader grubu ayrı draw pass olur
    for (auto& [shaderID, batch] : m_RenderQueueCache.batches)
    {
        shader& shader = ctx.shaderManager.get(shaderID);
        shader.use();

        // Kamera matrisleri
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        shader.setVec3("viewPos", cameraPosition);

        // Işık uzayı matrisini ve ürettiğimiz Shadow Map dokusunu ana shader'a gönderiyoruz
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        // Mevcut texture slotlarının çakışmaması için shadow map'i slot 2'ye bağlıyoruz
        // (diffuse = slot 0, normal = slot 1 demiştin)
        ctx.renderer.BindTextureSlot(2, ctx.shadowFBO.get_depth_texture());
        shader.setInt("shadowMap", 2);

        for (const auto& item : batch)
        {
            shader.setMat4("model", item.model);
            shader.setVec3("baseColor", item.base_color);
            shader.setVec2("u_TextureScale", item.texture_scale);

            shader.setInt("useTexture", item.diffuse_texture != 0);
            shader.setInt("useNormalMap", item.normal_texture != 0);

            if (item.normal_texture != 0)
            {
                shader.setFloat("normalStrength", item.normal_strength);
            }

            shader.setFloat("specularStrength", item.specular_strength);
            shader.setFloat("shininess", item.shininess);

            // Mesh draw call (renderer + texture manager üzerinden)
            ctx.renderer.Draw(item, ctx.textureManager);
        }
    }
}

/**
 * @brief ECS içinden render edilebilir objeleri toplar.
 *
 * İşlemler:
 * - Transform → model matrix oluşturma
 * - Frustum culling (görünmeyen objeleri eleme)
 * - Material çözümleme
 * - Render item üretimi
 */
void render_system::extract(
    const registry& reg,
    EngineContext& ctx,
    const glm::mat4& viewProj
)
{
    // Kamera görüş hacmi oluşturulur
    frustum f = buildFrustum(viewProj);

    // Culling hatalarını azaltmak için küçük bias
    constexpr float kCullBias = 0.01f;

    for (const auto& [e, mesh] : reg.get_all<mesh_component>())
    {
        const auto* transform = reg.get_component<transform_component>(e);
        const material_component* matComp = reg.get_component<material_component>(e);
        const auto* bounds = reg.get_component<bounds_component>(e);

        if (!transform) continue;

        // Model matrix (T * R * S)
        glm::mat4 model(1.0f);
        model = glm::translate(model, transform->position);
        model = glm::rotate(model, transform->rotation.x, glm::vec3(1, 0, 0));
        model = glm::rotate(model, transform->rotation.y, glm::vec3(0, 1, 0));
        model = glm::rotate(model, transform->rotation.z, glm::vec3(0, 0, 1));
        model = glm::scale(model, transform->scale);

        // Bounding sphere world space'e taşınır
        glm::vec3 localCenter = bounds ? bounds->localCenter : glm::vec3(0.0f);
        glm::vec3 worldCenter = glm::vec3(model * glm::vec4(localCenter, 1.0f));

        // Scale'e göre bounding radius hesaplanır
        float maxScale = glm::max(transform->scale.x,
            glm::max(transform->scale.y, transform->scale.z));

        float baseRadius = bounds ? bounds->localRadius : transform->boundingRadius;
        float radius = baseRadius * maxScale + kCullBias;

        // Frustum dışındaysa render edilmez
        if (!f.is_inside(worldCenter, radius))
            continue;

        // Render item oluşturulur
        render_item item;
        item.model = model;
        item.mesh_id = mesh.mesh_id;

        // Material çözümleme
        if (matComp)
        {
            const material* mat =
                ctx.assetManager.try_get<material>(matComp->material_id);

            if (mat)
            {
                item.shader_id = mat->shader_id;
                item.base_color = mat->base_color;
                item.normal_strength = mat->normal_strength;
                item.texture_scale = mat->texture_scale;
                item.diffuse_texture = mat->diffuse_texture;
                item.normal_texture = mat->normal_texture;

                item.specular_strength = mat->specular_strength;
                item.shininess = mat->shininess;
            }
            else
            {
                // Fallback material (debug magenta)
                item.shader_id = 0;
                item.base_color = glm::vec3(1.0f, 0.0f, 1.0f);
                item.diffuse_texture = 0;
                item.normal_texture = 0;
                item.texture_scale = glm::vec2(1.0f);

                item.specular_strength = 0.5f;
                item.shininess = 32.0f;
            }
        }
        else
        {
            // Material yoksa default değerler
            item.shader_id = 0;
            item.base_color = glm::vec3(1.0f);
            item.diffuse_texture = 0;
            item.normal_texture = 0;
            item.texture_scale = glm::vec2(1.0f);

            item.specular_strength = 0.5f;
            item.shininess = 32.0f;
        }

        // Cache'e eklenir (allocation yok → performans odaklı)
        m_RenderItemsCache.push_back(item);
    }
}

glm::mat4 render_system::calculate_light_space_matrix(const light_data& light, const glm::vec3& viewPos)
{
    // Projeksiyon kutusunu yeterli bir genişlikte tutuyoruz
    float near_plane = 0.1f, far_plane = 500.0f;
    glm::mat4 lightProjection = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, near_plane, far_plane);

    glm::vec3 lightPos = light.position;

    if (light.type == 0) // Directional Light
    {
        // ÇÖZÜM: Kameranın derinlik/yükseklik hareketinden etkilenmemek için 
        // Işığın merkez noktasını kameranın YATAY ($X, Z$) koordinatlarına eşitliyoruz.
        glm::vec3 shadowCenter = glm::vec3(viewPos.x, 0.0f, viewPos.z);

        // Sabit bir yükseklik ($100.0f$) belirliyoruz ve ışık yönünün tersine çekiyoruz
        lightPos = shadowCenter + glm::vec3(0.0f, 100.0f, 0.0f) - (glm::normalize(light.direction) * 150.0f);

        // Işığın tam olarak bu yatay merkeze bakmasını sağlıyoruz
        glm::vec3 target = shadowCenter + light.direction;
        glm::mat4 lightView = glm::lookAt(lightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));

        return lightProjection * lightView;
    }

    glm::vec3 target = lightPos + light.direction;
    glm::mat4 lightView = glm::lookAt(lightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}