/**
 * @file render_system.cpp
 * @brief Render sistemi I/O işlemleri, Frustum Culling ve OpenGL durum yönetim uygulaması.
 */

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
#include "../Core/Logger/logger.h"
#include <algorithm>

#include "../Renderer/MeshRenderer.h"

namespace {
    /**
     * @brief Render öğelerini GPU Shader ID'sine göre sıralamak için yardımcı fonksiyon.
     */
    bool compare_by_gpu_shader(const render_item& a, const render_item& b)
    {
        return a.gpu_shader_id < b.gpu_shader_id;
    }
} // anonymous namespace

render_system::render_system()
    : m_LightUBO(0)
{
}

void render_system::init()
{
    if (m_LightUBO != 0)
    {
        return;
    }

    const size_t uboSize = 16 + (sizeof(light_data) * MAX_LIGHT_COUNT);

    QLOG_INFO(renderer, "[RenderSystem] UBO Bellek Tahsisi (Boyut: {} byte, Isik Paketi Boyutu: {} byte)", uboSize, sizeof(light_data));


    // Global ışık UBO'sunun oluşturulması ve 0 numaralı bağlama noktasına (Binding Point) atanması
    glGenBuffers(1, &m_LightUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_LightUBO);
    glBufferData(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_LightUBO);

    QLOG_INFO(renderer, "[RenderSystem] Isik UBO'su basariyla ilklendirildi.");
}

render_system::~render_system()
{
    if (m_LightUBO != 0)
    {
        glDeleteBuffers(1, &m_LightUBO);
    }
}

void render_system::render(
    const registry& reg,
    const light_batch& lightBatch,
    EngineContext& ctx,
    const glm::mat4& view,
    const glm::mat4& projection,
    bool isSceneViewport)
{
    // Mevcut durumları (FBO ve Viewport) işlemler bitince geri dönmek üzere yedekle
    GLint previousFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_LightUBO);

    const glm::mat4 viewProj = projection * view;
    const glm::mat4 invView = glm::inverse(view);
    const glm::vec3 cameraPosition = glm::vec3(invView[3]);

    // 1. Işık Verilerinin GPU'ya Aktarımı (UBO)
    const auto& lights = lightBatch.lights;
    const int lightCount = std::min(static_cast<int>(lights.size()), static_cast<int>(MAX_LIGHT_COUNT));

    glBindBuffer(GL_UNIFORM_BUFFER, m_LightUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &lightCount);

    if (lightCount > 0)
    {
        glBufferSubData(GL_UNIFORM_BUFFER, 16, lightCount * sizeof(light_data), lights.data());
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // İlgili görünümün (Scene veya Game) istatistik sayacını sıfırla
    render_pass_stats& passStats = isSceneViewport ? m_SceneStats : m_GameStats;
    passStats = render_pass_stats{};
    passStats.active_lights = static_cast<uint32_t>(lightCount);

    // 2. Çıkarım (Extraction) ve Gruplama (Batching)
    m_RenderItemsCache.clear();
    extract(reg, ctx, viewProj, passStats);

    m_RenderQueueCache.batches.clear();

    // Varlıklar, Aria'dan çözümlenen GPU Shader ID'sine göre gruplanır (State değişimi minimizasyonu)
    for (const auto& item : m_RenderItemsCache)
    {
        m_RenderQueueCache.batches[item.gpu_shader_id].push_back(item);
    }

    passStats.shader_batches = static_cast<uint32_t>(m_RenderQueueCache.batches.size());

    // 3. Gölge Haritası için Işık Matrisinin Hesaplanması (Directional Light araması)
    glm::mat4 lightSpaceMatrix(1.0f);
    bool hasDirectionalLight = false;

    if (ctx.renderSettings.enable_shadows)
    {
        for (int i = 0; i < lightCount; ++i)
        {
            if (lights[i].type == 0) // Directional
            {
                lightSpaceMatrix = calculate_light_space_matrix(ctx, lights[i], cameraPosition);
                hasDirectionalLight = true;
                break;
            }
        }
    }

    passStats.shadow_pass_active = hasDirectionalLight;

    // ========================================================================
    // GÖLGE GEÇİŞİ (SHADOW PASS)
    // ========================================================================
    if (hasDirectionalLight)
    {
        ctx.shadowFBO.bind();

        const unsigned int shadowGLShaderID = ctx.aria_system->get_shader_id(ctx.shadowShaderHandle);
        const unsigned int skinnedShadowGLShaderID = ctx.aria_system->get_shader_id(ctx.skinnedShadowShaderHandle);

        unsigned int currentShaderID = 0;

        for (auto& [gpuShaderID, batch] : m_RenderQueueCache.batches)
        {
            for (const auto& item : batch)
            {
                unsigned int targetShader = item.is_skinned ? skinnedShadowGLShaderID : shadowGLShaderID;
                if (targetShader == 0) targetShader = shadowGLShaderID;

                if (currentShaderID != targetShader)
                {
                    currentShaderID = targetShader;
                    glUseProgram(currentShaderID);
                    glUniformMatrix4fv(glGetUniformLocation(currentShaderID, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
                }

                glUniformMatrix4fv(glGetUniformLocation(currentShaderID, "model"), 1, GL_FALSE, &item.model[0][0]);
                const unsigned int drawnIndices = ctx.renderer.Draw(item, ctx.aria_system);
                if (drawnIndices > 0)
                {
                    ++passStats.draw_calls_shadow;
                    passStats.triangles_shadow += drawnIndices / 3;
                }
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
        ctx.renderer.BindFramebuffer(previousFBO);
    }

    // ========================================================================
    // ANA ÇİZİM GEÇİŞİ (MAIN RENDER PASS)
    // ========================================================================
    ctx.renderer.SetViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    for (auto& [gpuShaderID, batch] : m_RenderQueueCache.batches)
    {
        glUseProgram(gpuShaderID);

        // Küresel Kamera ve Gölge Üniformaları
        glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform3fv(glGetUniformLocation(gpuShaderID, "viewPos"), 1, &cameraPosition[0]);
        glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        // Sis (Fog) Parametreleri:
        // isSceneViewport true ise (Sahne görünümü) ctx.fog.show_in_scene kontrol edilir.
        // isSceneViewport false ise (Oyun görünümü) doğrudan ctx.fog.enabled geçerlidir.
        const bool fogEnabled = ctx.fog.enabled && (isSceneViewport ? ctx.fog.show_in_scene : true);
        const glm::vec3 fogColor = ctx.fog.color;
        const float fogDensity = ctx.fog.density;

        glUniform1i(glGetUniformLocation(gpuShaderID, "u_FogEnabled"), fogEnabled ? 1 : 0);
        glUniform3fv(glGetUniformLocation(gpuShaderID, "u_FogColor"), 1, &fogColor[0]);
        glUniform1f(glGetUniformLocation(gpuShaderID, "u_FogDensity"), fogDensity);

        // Gölge haritası dokusunu 2. slota bağla
        ctx.renderer.BindTextureSlot(2, ctx.shadowFBO.get_depth_texture());
        glUniform1i(glGetUniformLocation(gpuShaderID, "shadowMap"), 2);

        // Paket (Batch) içindeki varlıkların bireysel (Per-Object) çizimi
        for (const auto& item : batch)
        {
            glUniformMatrix4fv(glGetUniformLocation(gpuShaderID, "model"), 1, GL_FALSE, &item.model[0][0]);
            glUniform3fv(glGetUniformLocation(gpuShaderID, "baseColor"), 1, &item.base_color[0]);
            glUniform2fv(glGetUniformLocation(gpuShaderID, "u_TextureScale"), 1, &item.texture_scale[0]);

            // Modern PBR Parametreleri
            glUniform1f(glGetUniformLocation(gpuShaderID, "u_Metallic"), item.metallic);
            glUniform1f(glGetUniformLocation(gpuShaderID, "u_Roughness"), item.roughness);
            glUniform1f(glGetUniformLocation(gpuShaderID, "u_AO"), item.ao);
            glUniform3fv(glGetUniformLocation(gpuShaderID, "u_EmissiveColor"), 1, &item.emissive_color[0]);
            glUniform1f(glGetUniformLocation(gpuShaderID, "u_EmissiveIntensity"), item.emissive_intensity);

            // Doku Slot ve Bayrak Ayarları
            glUniform1i(glGetUniformLocation(gpuShaderID, "useTexture"), item.diffuse_handle.is_valid());
            glUniform1i(glGetUniformLocation(gpuShaderID, "diffuseTex"), RenderConstants::DIFFUSE_TEXTURE_SLOT);

            glUniform1i(glGetUniformLocation(gpuShaderID, "useNormalMap"), item.normal_handle.is_valid());
            glUniform1i(glGetUniformLocation(gpuShaderID, "normalTex"), RenderConstants::NORMAL_TEXTURE_SLOT);
            if (item.normal_handle.is_valid())
            {
                glUniform1f(glGetUniformLocation(gpuShaderID, "normalStrength"), item.normal_strength);
            }

            glUniform1i(glGetUniformLocation(gpuShaderID, "u_UseMetallicMap"), item.metallic_handle.is_valid());
            glUniform1i(glGetUniformLocation(gpuShaderID, "metallicTex"), RenderConstants::METALLIC_TEXTURE_SLOT);

            glUniform1i(glGetUniformLocation(gpuShaderID, "u_UseRoughnessMap"), item.roughness_handle.is_valid());
            glUniform1i(glGetUniformLocation(gpuShaderID, "roughnessTex"), RenderConstants::ROUGHNESS_TEXTURE_SLOT);

            glUniform1i(glGetUniformLocation(gpuShaderID, "u_UseAOMap"), item.ao_handle.is_valid());
            glUniform1i(glGetUniformLocation(gpuShaderID, "aoTex"), RenderConstants::AO_TEXTURE_SLOT);

            glUniform1i(glGetUniformLocation(gpuShaderID, "u_UseEmissiveMap"), item.emissive_handle.is_valid());
            glUniform1i(glGetUniformLocation(gpuShaderID, "emissiveTex"), RenderConstants::EMISSIVE_TEXTURE_SLOT);

            // Geriye dönük uyumluluk
            glUniform1f(glGetUniformLocation(gpuShaderID, "specularStrength"), item.specular_strength);
            glUniform1f(glGetUniformLocation(gpuShaderID, "shininess"), item.shininess);

            glUniform1i(glGetUniformLocation(gpuShaderID, "u_RenderMode"), static_cast<int>(item.mode));
            glUniform1f(glGetUniformLocation(gpuShaderID, "u_AlphaThreshold"), item.alpha_threshold);

            const unsigned int drawnIndices = ctx.renderer.Draw(item, ctx.aria_system);
            if (drawnIndices > 0)
            {
                ++passStats.draw_calls_main;
                passStats.triangles_main += drawnIndices / 3;
            }
        }
    }

    passStats.total_draw_calls = passStats.draw_calls_main + passStats.draw_calls_shadow;
}

void render_system::extract(
    const registry& reg,
    EngineContext& ctx,
    const glm::mat4& viewProj,
    render_pass_stats& outStats)
{
    const frustum f = buildFrustum(viewProj);
    constexpr float kCullBias = 0.01f;

    for (const auto& [e, meshComp] : reg.get_all<mesh_component>())
    {
        const auto* transform = reg.get_component<transform_component>(e);
        const auto* matComp = reg.get_component<material_component>(e);

        if (!transform)
        {
            continue;
        }

        // 1. Model Matrisinin Hesaplanması
        const glm::mat4 model = ecs_transform::get_world_matrix(e, reg);

        // 2. Frustum Culling (Görüş Alanı Kırpması)
        const aria::submesh_runtime_info meshInfo =
            ctx.aria_system->get_submesh_info(
                meshComp.model_handle,
                meshComp.sub_mesh_index);

        const glm::vec3 localCenter = meshInfo.local_center;
        const glm::vec4 homogeneousCenter = model * glm::vec4(localCenter, 1.0f);
        const glm::vec3 worldCenter = glm::vec3(homogeneousCenter) / homogeneousCenter.w;

        const glm::vec3 absoluteScale = glm::abs(transform->scale);
        const float maxScale = glm::max(
            absoluteScale.x,
            glm::max(absoluteScale.y, absoluteScale.z));

        const float radius = (meshInfo.local_radius * maxScale) + ctx.renderSettings.cull_bias;

        if (ctx.renderSettings.enable_frustum_culling && !f.is_inside(worldCenter, radius))
        {
            ++outStats.culled_submeshes;
            continue;
        }
        ++outStats.visible_submeshes;

        // 3. Render Öğe Verilerinin Toplanması
        render_item item;
        item.model = model;
        item.model_handle = meshComp.model_handle;
        item.sub_mesh_index = meshComp.sub_mesh_index;

        const unsigned int defaultShaderID = ctx.aria_system->get_shader_id(ctx.defaultShaderHandle);

        if (matComp && matComp->material_handle.is_valid())
        {
            const auto* mat = ctx.aria_system->get_material_data(matComp->material_handle);

            if (mat)
            {
                if (mat->shader_handle.is_valid())
                {
                    item.gpu_shader_id = ctx.aria_system->get_shader_id(mat->shader_handle);
                }
                else
                {
                    item.gpu_shader_id = defaultShaderID;
                }

                item.mode = mat->mode;
                item.alpha_threshold = mat->alpha_threshold;
                item.base_color = mat->base_color;
                item.metallic = mat->metallic;
                item.roughness = mat->roughness;
                item.ao = mat->ao;
                item.emissive_color = mat->emissive_color;
                item.emissive_intensity = mat->emissive_intensity;

                item.normal_strength = mat->normal_strength;
                item.texture_scale = mat->texture_scale;
                item.diffuse_handle = mat->diffuse_texture_handle;
                item.normal_handle = mat->normal_texture_handle;
                item.metallic_handle = mat->metallic_texture_handle;
                item.roughness_handle = mat->roughness_texture_handle;
                item.ao_handle = mat->ao_texture_handle;
                item.emissive_handle = mat->emissive_texture_handle;
                item.specular_handle = mat->specular_texture_handle;
                item.specular_strength = mat->specular_strength;
                item.shininess = mat->shininess;
            }
            else
            {
                // Varlık çözümlemesi başarısız olursa varsayılan lit shader atanır
                item.gpu_shader_id = defaultShaderID;
                item.mode = aria::render_mode::opaque;
                item.alpha_threshold = 0.1f;
                item.base_color = glm::vec3(1.0f);
                item.metallic = 0.0f;
                item.roughness = 0.5f;
                item.ao = 1.0f;
                item.emissive_color = glm::vec3(0.0f);
                item.emissive_intensity = 0.0f;
                item.texture_scale = glm::vec2(1.0f);
                item.specular_strength = 0.5f;
                item.shininess = 32.0f;
            }
        }
        else
        {
            // Materyal bileşeni yoksa varsayılan lit shader atanır
            item.gpu_shader_id = defaultShaderID;
            item.mode = aria::render_mode::opaque;
            item.alpha_threshold = 0.1f;
            item.base_color = glm::vec3(1.0f);
            item.metallic = 0.0f;
            item.roughness = 0.5f;
            item.ao = 1.0f;
            item.emissive_color = glm::vec3(0.0f);
            item.emissive_intensity = 0.0f;
            item.texture_scale = glm::vec2(1.0f);
            item.specular_strength = 0.5f;
            item.shininess = 32.0f;
        }

        m_RenderItemsCache.push_back(item);
    }

    // 4. İskeletli (Skinned) Mesh Varlıklarının Çıkarımı
    for (const auto& [e, skinnedMeshComp] : reg.get_all<skinned_mesh_component>())
    {
        const auto* transform = reg.get_component<transform_component>(e);
        const auto* matComp = reg.get_component<material_component>(e);
        const auto* skelComp = reg.get_component<skeleton_component>(e);

        if (!transform)
        {
            continue;
        }

        // Model Matrisinin Hesaplanması
        const glm::mat4 model = ecs_transform::get_world_matrix(e, reg);

        const auto* skinnedModelData = ctx.aria_system->get_skinned_model_data(skinnedMeshComp.model_handle);
        if (!skinnedModelData || skinnedModelData->sub_meshes.empty())
        {
            continue;
        }

        const size_t totalSubMeshes = skinnedModelData->sub_meshes.size();

        for (size_t subIdx = 0; subIdx < totalSubMeshes; ++subIdx)
        {
            // Frustum Culling
            const aria::submesh_runtime_info meshInfo =
                ctx.aria_system->get_skinned_submesh_info(
                    skinnedMeshComp.model_handle,
                    subIdx);

            const glm::vec3 localCenter = meshInfo.local_center;
            const glm::vec4 homogeneousCenter = model * glm::vec4(localCenter, 1.0f);
            const glm::vec3 worldCenter = glm::vec3(homogeneousCenter) / homogeneousCenter.w;

            const glm::vec3 absoluteScale = glm::abs(transform->scale);
            const float maxScale = glm::max(
                absoluteScale.x,
                glm::max(absoluteScale.y, absoluteScale.z));

            const float radius = (meshInfo.local_radius * maxScale) + ctx.renderSettings.cull_bias;

            if (ctx.renderSettings.enable_frustum_culling && !f.is_inside(worldCenter, radius))
            {
                ++outStats.culled_submeshes;
                continue;
            }
            ++outStats.visible_submeshes;

            render_item item;
            item.is_skinned = true;
            item.model = model;
            item.skinned_model_handle = skinnedMeshComp.model_handle;
            item.sub_mesh_index = subIdx;

            if (skelComp && !skelComp->skinning_palette.empty())
            {
                item.bone_matrices = &skelComp->skinning_palette;
            }
            else
            {
                item.bone_matrices = nullptr;
            }

            const unsigned int defaultSkinnedShaderID = ctx.aria_system->get_shader_id(ctx.skinnedShaderHandle);

            if (matComp && matComp->material_handle.is_valid())
            {
                const auto* mat = ctx.aria_system->get_material_data(matComp->material_handle);

                if (mat)
                {
                    if (mat->shader_handle.is_valid())
                    {
                        item.gpu_shader_id = ctx.aria_system->get_shader_id(mat->shader_handle);
                    }
                    else
                    {
                        item.gpu_shader_id = defaultSkinnedShaderID;
                    }

                    item.mode = mat->mode;
                    item.alpha_threshold = mat->alpha_threshold;
                    item.base_color = mat->base_color;
                    item.metallic = mat->metallic;
                    item.roughness = mat->roughness;
                    item.ao = mat->ao;
                    item.emissive_color = mat->emissive_color;
                    item.emissive_intensity = mat->emissive_intensity;

                    item.normal_strength = mat->normal_strength;
                    item.texture_scale = mat->texture_scale;
                    item.diffuse_handle = mat->diffuse_texture_handle;
                    item.normal_handle = mat->normal_texture_handle;
                    item.metallic_handle = mat->metallic_texture_handle;
                    item.roughness_handle = mat->roughness_texture_handle;
                    item.ao_handle = mat->ao_texture_handle;
                    item.emissive_handle = mat->emissive_texture_handle;
                    item.specular_handle = mat->specular_texture_handle;
                    item.specular_strength = mat->specular_strength;
                    item.shininess = mat->shininess;
                }
                else
                {
                    item.gpu_shader_id = defaultSkinnedShaderID;
                    item.mode = aria::render_mode::opaque;
                    item.alpha_threshold = 0.1f;
                    item.base_color = glm::vec3(1.0f, 0.0f, 1.0f);
                    item.metallic = 0.0f;
                    item.roughness = 0.5f;
                    item.ao = 1.0f;
                    item.emissive_color = glm::vec3(0.0f);
                    item.emissive_intensity = 0.0f;
                    item.texture_scale = glm::vec2(1.0f);
                    item.specular_strength = 0.5f;
                    item.shininess = 32.0f;
                }
            }
            else
            {
                item.gpu_shader_id = defaultSkinnedShaderID;
                item.mode = aria::render_mode::opaque;
                item.alpha_threshold = 0.1f;
                item.base_color = glm::vec3(1.0f);
                item.metallic = 0.0f;
                item.roughness = 0.5f;
                item.ao = 1.0f;
                item.emissive_color = glm::vec3(0.0f);
                item.emissive_intensity = 0.0f;
                item.texture_scale = glm::vec2(1.0f);
                item.specular_strength = 0.5f;
                item.shininess = 32.0f;
            }

            m_RenderItemsCache.push_back(item);
        }
    }
}

glm::mat4 render_system::calculate_light_space_matrix(const EngineContext& ctx, const light_data& light, const glm::vec3& viewPos)
{
    const float near_plane = ctx.renderSettings.shadow_near_plane;
    const float far_plane = ctx.renderSettings.shadow_far_plane;
    const float ortho_size = ctx.renderSettings.shadow_ortho_size;
    const glm::mat4 lightProjection = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, near_plane, far_plane);

    glm::vec3 lightPos = light.position;

    if (light.type == 0) // Yönlü Işık (Directional Light)
    {
        // Kameranın dikey hareketlerinden etkilenmemesi ve gölge sınırlarının 
        // kamerayı dengeli takip etmesi için ışık merkezi kameranın yatay düzlemine (X, Z) sabitlenir.
        const glm::vec3 shadowCenter = glm::vec3(viewPos.x, 0.0f, viewPos.z);

        // Belirlenen yükseklik ve takip mesafesine göre ışık yönünün tersine konumlandırılır
        lightPos = shadowCenter + glm::vec3(0.0f, ctx.renderSettings.shadow_camera_height, 0.0f)
            - (glm::normalize(light.direction) * ctx.renderSettings.shadow_distance);

        const glm::vec3 target = shadowCenter + light.direction;
        const glm::mat4 lightView = glm::lookAt(lightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));

        return lightProjection * lightView;
    }

    const glm::vec3 target = lightPos + light.direction;
    const glm::mat4 lightView = glm::lookAt(lightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}