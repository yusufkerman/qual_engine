#include "render_system.h"
#include "../Engine/engine_context.h" 
#include "../ECS/registry.h"
#include "../ECS/components.h"
#include "../ECS/transform_utils.h"
#include "../Renderer/frustum.h"
#include "../Renderer/material.h"
#include "../shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm> // std::min için

// MeshRenderer enum/sabitleri (mesh id vb.)
#include "../Renderer/MeshRenderer.h"


namespace {
    bool compare_by_gpu_shader(const render_item& a, const render_item& b) {
        return a.gpu_shader_id < b.gpu_shader_id;
    }
}

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

    // === YENİ EKLENEN DEBUG SATIRI ===
    std::cout << "[UBO CHECK] Allocated uboSize = " << uboSize
        << " sizeof(light_data) = " << sizeof(light_data) << "\n";
    // ==================================

    // Not: UBO sistemin kendi iç yönetimi olduğu için buradaki ham çağrılar kalmıştır.
    glGenBuffers(1, &m_LightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_LightUBO);
    glBufferData(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_DRAW);


    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_LightUBO);

    std::cout << "[RenderSystem] Light UBO successfully initialized." << std::endl;
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

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_LightUBO);

    glm::mat4 viewProj = projection * view;
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPosition = glm::vec3(invView[3]);

    // Işık UBO işlemleri
    const auto& lights = lightBatch.lights;
    int lightCount = std::min(static_cast<int>(lights.size()), static_cast<int>(MAX_LIGHT_COUNT));

    glBindBuffer(GL_UNIFORM_BUFFER, m_LightUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &lightCount);
    if (lightCount > 0) {
        

        glBufferSubData(GL_UNIFORM_BUFFER, 16, lightCount * sizeof(light_data), lights.data());
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);



    // Frame başı render cache temizlenir ve ECS'den objeler toplanır
    m_RenderItemsCache.clear();
    extract(reg, ctx, viewProj);

    // Shader bazlı batch map temizlenir
    m_RenderQueueCache.batches.clear();

    // 🚀 DOĞRU GRUPLAMA: Nesneler Aria'dan gelen ham gpu_shader_id'ye göre kuyruğa (map'e) eklenir
    for (const auto& item : m_RenderItemsCache)
        m_RenderQueueCache.batches[item.gpu_shader_id].push_back(item);

    // Shadow Map Matris Hesabı
    glm::mat4 lightSpaceMatrix(1.0f);
    bool hasDirectionalLight = false;
    for (int i = 0; i < lightCount; ++i) {
        if (lights[i].type == 0) {
            lightSpaceMatrix = calculate_light_space_matrix(lights[i], cameraPosition);
            hasDirectionalLight = true;
            break;
        }
    }

    // ------------------------------------------------------------------------
    // PASS 1: SHADOW MAP RENDER (Gölge Pass'i)
    // ------------------------------------------------------------------------
    if (hasDirectionalLight)
    {
        ctx.shadowFBO.bind();

        // ========================================================================
        // 🚀 ARIA SHADOW SHADER KATMANI
        // ========================================================================
        // Eski shadowShaderID yerine Aria Cache'den GPU üzerindeki hazır shader ID'sini çekiyoruz
        unsigned int shadowGLShaderID = ctx.aria_system->get_shader_id(ctx.shadowShaderHandle);
        glUseProgram(shadowGLShaderID);

        glUniformMatrix4fv(glGetUniformLocation(shadowGLShaderID, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        for (auto& [gpuShaderID, batch] : m_RenderQueueCache.batches)
        {
            for (const auto& item : batch)
            {
                glUniformMatrix4fv(glGetUniformLocation(shadowGLShaderID, "model"), 1, GL_FALSE, &item.model[0][0]);

                // 🚀 ARIA İLE ÇİZİM (Sıfır ek yük, tamamen optimize)
                ctx.renderer.Draw(item, ctx.aria_system);
            }
        }
        ctx.renderer.BindFramebuffer(previousFBO);
    }
    // ------------------------------------------------------------------------
    // PASS 2: ANA RENDER PASS
    // ------------------------------------------------------------------------
    ctx.renderer.SetViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    for (auto& [gpuShaderID, batch] : m_RenderQueueCache.batches)
    {
        // 🚀 Shader ID'yi doğrudan GL'e veriyoruz, "shader" sınıfı sarmalayıcısı 
        // gerekiyorsa burada sadece ID'sini geçiriyoruz.
        glUseProgram(gpuShaderID);

        GLint current;

        glGetIntegerv(GL_CURRENT_PROGRAM, &current);


        // Uniform set işlemleri (senin tıkır tıkır çalışan fonksiyonların)
        glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform3fv(glGetUniformLocation(gpuShaderID, "viewPos"), 1, &cameraPosition[0]);
        glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        // Shadow Map bağlama (Renderer üzerinden)
        ctx.renderer.BindTextureSlot(2, ctx.shadowFBO.get_depth_texture());
        glUniform1i(glGetUniformLocation(gpuShaderID, "shadowMap"), 2);

        for (const auto& item : batch)
        {
            glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "model"), 1, GL_FALSE, &item.model[0][0]);
            glUniform3fv(glGetUniformLocation(gpuShaderID, "baseColor"), 1, &item.base_color[0]);
            glUniform2fv(glGetUniformLocation(gpuShaderID, "u_TextureScale"), 1, &item.texture_scale[0]);

            glUniform1i(glGetUniformLocation(gpuShaderID, "useTexture"), item.diffuse_handle.is_valid());
            glUniform1i(glGetUniformLocation(gpuShaderID, "useNormalMap"), item.normal_handle.is_valid());

            if (item.normal_handle.is_valid()) {
                glUniform1f(glGetUniformLocation(gpuShaderID, "normalStrength"), item.normal_strength);
            }
            glUniform1f(glGetUniformLocation(gpuShaderID, "specularStrength"), item.specular_strength);
            glUniform1f(glGetUniformLocation(gpuShaderID, "shininess"), item.shininess);

            glUniform1i(glGetUniformLocation(gpuShaderID, "u_RenderMode"), static_cast<int>(item.mode));
            glUniform1f(glGetUniformLocation(gpuShaderID, "u_AlphaThreshold"), item.alpha_threshold);

            // 🚀 ÇİZİM TETİKLENİYOR
            ctx.renderer.Draw(item, ctx.aria_system);
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
    frustum f = buildFrustum(viewProj);
    constexpr float kCullBias = 0.01f;

    for (const auto& [e, meshComp] : reg.get_all<mesh_component>())
    {
        const auto* transform = reg.get_component<transform_component>(e);
        const auto* matComp = reg.get_component<material_component>(e);

        if (!transform)
            continue;

        // ------------------------------------------------------------
        // Model Matrix
        // ------------------------------------------------------------
        glm::mat4 model = ecs_transform::get_world_matrix(e, reg);

        // ------------------------------------------------------------
        // Bounds artık doğrudan Aria Asset sisteminden geliyor
        // ------------------------------------------------------------
        aria::submesh_runtime_info meshInfo =
            ctx.aria_system->get_submesh_info(
                meshComp.model_handle,
                meshComp.sub_mesh_index);

        glm::vec3 localCenter = meshInfo.local_center;

        glm::vec4 homogeneousCenter = model * glm::vec4(localCenter, 1.0f);
        glm::vec3 worldCenter = glm::vec3(homogeneousCenter) / homogeneousCenter.w;

        glm::vec3 absoluteScale = glm::abs(transform->scale);
        float maxScale = glm::max(
            absoluteScale.x,
            glm::max(absoluteScale.y, absoluteScale.z));

        float radius = (meshInfo.local_radius * maxScale) + kCullBias;

        // Frustum dışında ise render kuyruğuna alma
        if (!f.is_inside(worldCenter, radius))
            continue;

        // ------------------------------------------------------------
        // Render Item oluştur
        // ------------------------------------------------------------
        render_item item;
        item.model = model;

        item.model_handle = meshComp.model_handle;
        item.sub_mesh_index = meshComp.sub_mesh_index;

        if (matComp && matComp->material_handle.is_valid())
        {
            const auto* mat =
                ctx.aria_system->get_material_data(matComp->material_handle);

            if (mat)
            {
                if (mat->shader_handle.is_valid())
                {
                    item.gpu_shader_id =
                        ctx.aria_system->get_shader_id(mat->shader_handle);
                }
                else
                {
                    item.gpu_shader_id =
                        ctx.aria_system->get_shader_id(
                            aria::handle<aria::shader_asset>());
                }

                item.mode = mat->mode;
                item.alpha_threshold = mat->alpha_threshold;

                item.base_color = mat->base_color;
                item.normal_strength = mat->normal_strength;
                item.texture_scale = mat->texture_scale;
                item.diffuse_handle = mat->diffuse_texture_handle;
                item.normal_handle = mat->normal_texture_handle;
                item.specular_handle = mat->specular_texture_handle;
                item.specular_strength = mat->specular_strength;
                item.shininess = mat->shininess;
            }
            else
            {
                item.gpu_shader_id =
                    ctx.aria_system->get_shader_id(
                        aria::handle<aria::shader_asset>());

                item.mode = aria::render_mode::opaque;
                item.alpha_threshold = 0.1f;

                item.base_color = glm::vec3(1.0f, 0.0f, 1.0f);
                item.texture_scale = glm::vec2(1.0f);
                item.specular_strength = 0.5f;
                item.shininess = 32.0f;
            }
        }
        else
        {
            item.gpu_shader_id =
                ctx.aria_system->get_shader_id(
                    aria::handle<aria::shader_asset>());

            item.mode = aria::render_mode::opaque;
            item.alpha_threshold = 0.1f;

            item.base_color = glm::vec3(1.0f);
            item.texture_scale = glm::vec2(1.0f);
            item.specular_strength = 0.5f;
            item.shininess = 32.0f;
        }

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