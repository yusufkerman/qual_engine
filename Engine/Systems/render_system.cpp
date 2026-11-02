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

void render_system::clear_uniform_cache() noexcept
{
    m_ShaderUniformCache.clear();
    m_ShadowUniformCache.clear();
}

const shader_uniform_cache& render_system::get_shader_uniforms(unsigned int shaderID)
{
    auto it = m_ShaderUniformCache.find(shaderID);
    if (it != m_ShaderUniformCache.end())
    {
        return it->second;
    }

    shader_uniform_cache c;
    c.u_view = glGetUniformLocation(shaderID, "view");
    c.u_projection = glGetUniformLocation(shaderID, "projection");
    c.u_viewPos = glGetUniformLocation(shaderID, "viewPos");
    c.u_lightSpaceMatrix = glGetUniformLocation(shaderID, "lightSpaceMatrix");
    c.u_FogEnabled = glGetUniformLocation(shaderID, "u_FogEnabled");
    c.u_FogColor = glGetUniformLocation(shaderID, "u_FogColor");
    c.u_FogDensity = glGetUniformLocation(shaderID, "u_FogDensity");
    c.u_shadowMap = glGetUniformLocation(shaderID, "shadowMap");

    c.u_model = glGetUniformLocation(shaderID, "model");
    c.u_baseColor = glGetUniformLocation(shaderID, "baseColor");
    c.u_TextureScale = glGetUniformLocation(shaderID, "u_TextureScale");
    c.u_Metallic = glGetUniformLocation(shaderID, "u_Metallic");
    c.u_Roughness = glGetUniformLocation(shaderID, "u_Roughness");
    c.u_AO = glGetUniformLocation(shaderID, "u_AO");
    c.u_EmissiveColor = glGetUniformLocation(shaderID, "u_EmissiveColor");
    c.u_EmissiveIntensity = glGetUniformLocation(shaderID, "u_EmissiveIntensity");
    c.u_useTexture = glGetUniformLocation(shaderID, "useTexture");
    c.u_diffuseTex = glGetUniformLocation(shaderID, "diffuseTex");
    c.u_useNormalMap = glGetUniformLocation(shaderID, "useNormalMap");
    c.u_normalTex = glGetUniformLocation(shaderID, "normalTex");
    c.u_normalStrength = glGetUniformLocation(shaderID, "normalStrength");
    c.u_UseMetallicMap = glGetUniformLocation(shaderID, "u_UseMetallicMap");
    c.u_metallicTex = glGetUniformLocation(shaderID, "metallicTex");
    c.u_UseRoughnessMap = glGetUniformLocation(shaderID, "u_UseRoughnessMap");
    c.u_roughnessTex = glGetUniformLocation(shaderID, "roughnessTex");
    c.u_UseAOMap = glGetUniformLocation(shaderID, "u_UseAOMap");
    c.u_aoTex = glGetUniformLocation(shaderID, "aoTex");
    c.u_UseEmissiveMap = glGetUniformLocation(shaderID, "u_UseEmissiveMap");
    c.u_emissiveTex = glGetUniformLocation(shaderID, "emissiveTex");
    c.u_specularStrength = glGetUniformLocation(shaderID, "specularStrength");
    c.u_shininess = glGetUniformLocation(shaderID, "shininess");
    c.u_RenderMode = glGetUniformLocation(shaderID, "u_RenderMode");
    c.u_AlphaThreshold = glGetUniformLocation(shaderID, "u_AlphaThreshold");
    c.u_EnableCSM = glGetUniformLocation(shaderID, "u_EnableCSM");
    c.u_ShadowMapArray = glGetUniformLocation(shaderID, "u_ShadowMapArray");
    c.u_CascadeCount = glGetUniformLocation(shaderID, "u_CascadeCount");
    c.u_CascadePlaneDistances = glGetUniformLocation(shaderID, "u_CascadePlaneDistances[0]");
    if (c.u_CascadePlaneDistances < 0) c.u_CascadePlaneDistances = glGetUniformLocation(shaderID, "u_CascadePlaneDistances");
    c.u_CascadeLightSpaceMatrices = glGetUniformLocation(shaderID, "u_CascadeLightSpaceMatrices[0]");
    if (c.u_CascadeLightSpaceMatrices < 0) c.u_CascadeLightSpaceMatrices = glGetUniformLocation(shaderID, "u_CascadeLightSpaceMatrices");
    c.u_UseSkyboxReflection = glGetUniformLocation(shaderID, "u_UseSkyboxReflection");
    c.u_SkyboxTex = glGetUniformLocation(shaderID, "u_SkyboxTex");
    c.u_PS1VertexSnapping = glGetUniformLocation(shaderID, "u_PS1VertexSnapping");
    c.u_PS1AffineMapping = glGetUniformLocation(shaderID, "u_PS1AffineMapping");
    c.u_PS1ColorDepth = glGetUniformLocation(shaderID, "u_PS1ColorDepth");
    c.u_PS1Dithering = glGetUniformLocation(shaderID, "u_PS1Dithering");
    c.u_PS1DitherStrength = glGetUniformLocation(shaderID, "u_PS1DitherStrength");

    glUseProgram(shaderID);
    if (c.u_diffuseTex >= 0) glUniform1i(c.u_diffuseTex, RenderConstants::DIFFUSE_TEXTURE_SLOT);
    if (c.u_normalTex >= 0) glUniform1i(c.u_normalTex, RenderConstants::NORMAL_TEXTURE_SLOT);
    if (c.u_shadowMap >= 0) glUniform1i(c.u_shadowMap, RenderConstants::SHADOW_TEXTURE_SLOT);
    if (c.u_metallicTex >= 0) glUniform1i(c.u_metallicTex, RenderConstants::METALLIC_TEXTURE_SLOT);
    if (c.u_roughnessTex >= 0) glUniform1i(c.u_roughnessTex, RenderConstants::ROUGHNESS_TEXTURE_SLOT);
    if (c.u_aoTex >= 0) glUniform1i(c.u_aoTex, RenderConstants::AO_TEXTURE_SLOT);
    if (c.u_emissiveTex >= 0) glUniform1i(c.u_emissiveTex, RenderConstants::EMISSIVE_TEXTURE_SLOT);
    if (c.u_ShadowMapArray >= 0) glUniform1i(c.u_ShadowMapArray, RenderConstants::CSM_SHADOW_TEXTURE_SLOT);

    return m_ShaderUniformCache.emplace(shaderID, c).first->second;
}

const shadow_uniform_cache& render_system::get_shadow_uniforms(unsigned int shaderID)
{
    auto it = m_ShadowUniformCache.find(shaderID);
    if (it != m_ShadowUniformCache.end())
    {
        return it->second;
    }

    shadow_uniform_cache c;
    c.u_lightSpaceMatrix = glGetUniformLocation(shaderID, "lightSpaceMatrix");
    c.u_model = glGetUniformLocation(shaderID, "model");

    return m_ShadowUniformCache.emplace(shaderID, c).first->second;
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

    // 3. Gölge Haritası ve Hacimsel Işıklandırma için Işık Matrisinin Hesaplanması (Directional Light araması)
    m_HasDirectionalLight = false;
    m_DirectionalLight = {};
    m_DirectionalLightSpaceMatrix = glm::mat4(1.0f);
    m_CSMCascades.clear();

    if (ctx.renderSettings.enable_shadows || ctx.renderSettings.enable_volumetric_lighting)
    {
        for (int i = 0; i < lightCount; ++i)
        {
            if (lights[i].type == 0) // Directional
            {
                m_DirectionalLight = lights[i];
                m_DirectionalLightSpaceMatrix = calculate_light_space_matrix(ctx, lights[i], cameraPosition);
                m_HasDirectionalLight = true;
                break;
            }
        }
    }

    if (m_HasDirectionalLight && ctx.renderSettings.enable_csm)
    {
        m_CSMCascades = calculate_csm_cascades(ctx, m_DirectionalLight, view, projection, cameraPosition);
    }

    passStats.shadow_pass_active = m_HasDirectionalLight && ctx.renderSettings.enable_shadows;

    // ========================================================================
    // GÖLGE GEÇİŞİ (SHADOW PASS)
    // ========================================================================
    if (m_HasDirectionalLight && ctx.renderSettings.enable_shadows)
    {
        const unsigned int shadowGLShaderID = ctx.aria_system->get_shader_id(ctx.shadowShaderHandle);
        const unsigned int skinnedShadowGLShaderID = ctx.aria_system->get_shader_id(ctx.skinnedShadowShaderHandle);

        if (ctx.renderSettings.enable_csm && !m_CSMCascades.empty())
        {
            for (size_t c = 0; c < m_CSMCascades.size(); ++c)
            {
                ctx.shadowFBO.bind_cascade(static_cast<unsigned int>(c));

                unsigned int currentShaderID = 0;
                const shadow_uniform_cache* currentShadowCache = nullptr;

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
                            currentShadowCache = &get_shadow_uniforms(currentShaderID);
                            if (currentShadowCache->u_lightSpaceMatrix >= 0)
                            {
                                glUniformMatrix4fv(currentShadowCache->u_lightSpaceMatrix, 1, GL_FALSE, &m_CSMCascades[c].light_space_matrix[0][0]);
                            }
                        }

                        if (currentShadowCache && currentShadowCache->u_model >= 0)
                        {
                            glUniformMatrix4fv(currentShadowCache->u_model, 1, GL_FALSE, &item.model[0][0]);
                        }
                        const unsigned int drawnIndices = ctx.renderer.Draw(item, ctx.aria_system);
                        if (drawnIndices > 0)
                        {
                            ++passStats.draw_calls_shadow;
                            passStats.triangles_shadow += drawnIndices / 3;
                        }
                    }
                }
            }
        }
        else
        {
            ctx.shadowFBO.bind();

            unsigned int currentShaderID = 0;
            const shadow_uniform_cache* currentShadowCache = nullptr;

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
                        currentShadowCache = &get_shadow_uniforms(currentShaderID);
                        if (currentShadowCache->u_lightSpaceMatrix >= 0)
                        {
                            glUniformMatrix4fv(currentShadowCache->u_lightSpaceMatrix, 1, GL_FALSE, &m_DirectionalLightSpaceMatrix[0][0]);
                        }
                    }

                    if (currentShadowCache && currentShadowCache->u_model >= 0)
                    {
                        glUniformMatrix4fv(currentShadowCache->u_model, 1, GL_FALSE, &item.model[0][0]);
                    }
                    const unsigned int drawnIndices = ctx.renderer.Draw(item, ctx.aria_system);
                    if (drawnIndices > 0)
                    {
                        ++passStats.draw_calls_shadow;
                        passStats.triangles_shadow += drawnIndices / 3;
                    }
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

    const bool hasSkybox = ctx.renderSettings.enable_skybox && (ctx.skyboxTexture != 0);
    if (hasSkybox)
    {
        glActiveTexture(GL_TEXTURE0 + RenderConstants::SKYBOX_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.skyboxTexture);
    }

    for (auto& [gpuShaderID, batch] : m_RenderQueueCache.batches)
    {
        glUseProgram(gpuShaderID);
        const shader_uniform_cache& u = get_shader_uniforms(gpuShaderID);

        // Küresel Kamera ve Gölge Üniformaları (Önbellek üzerinden sıfır string araması)
        if (u.u_view >= 0) glUniformMatrix4fv(u.u_view, 1, GL_FALSE, &view[0][0]);
        if (u.u_projection >= 0) glUniformMatrix4fv(u.u_projection, 1, GL_FALSE, &projection[0][0]);
        if (u.u_viewPos >= 0) glUniform3fv(u.u_viewPos, 1, &cameraPosition[0]);
        if (u.u_lightSpaceMatrix >= 0) glUniformMatrix4fv(u.u_lightSpaceMatrix, 1, GL_FALSE, &m_DirectionalLightSpaceMatrix[0][0]);

        // Sis (Fog) Parametreleri:
        const bool fogEnabled = ctx.fog.enabled && (isSceneViewport ? ctx.fog.show_in_scene : true);
        const glm::vec3 fogColor = ctx.fog.color;
        const float fogDensity = ctx.fog.density;

        if (u.u_FogEnabled >= 0) glUniform1i(u.u_FogEnabled, fogEnabled ? 1 : 0);
        if (u.u_FogColor >= 0) glUniform3fv(u.u_FogColor, 1, &fogColor[0]);
        if (u.u_FogDensity >= 0) glUniform1f(u.u_FogDensity, fogDensity);

        // Çevre Yansıması (Skybox IBL Reflection)
        if (u.u_UseSkyboxReflection >= 0) glUniform1i(u.u_UseSkyboxReflection, hasSkybox ? 1 : 0);
        if (hasSkybox && u.u_SkyboxTex >= 0) glUniform1i(u.u_SkyboxTex, RenderConstants::SKYBOX_TEXTURE_SLOT);

        // Gölge haritası dokusunu SHADOW_TEXTURE_SLOT (2) slota bağla (Standart 2D fallback)
        ctx.renderer.BindTextureSlot(RenderConstants::SHADOW_TEXTURE_SLOT, ctx.shadowFBO.get_depth_texture());
        if (u.u_shadowMap >= 0) glUniform1i(u.u_shadowMap, RenderConstants::SHADOW_TEXTURE_SLOT);

        // Kaskatlı gölge haritası doku dizisini (GL_TEXTURE_2D_ARRAY) CSM_SHADOW_TEXTURE_SLOT (8) slota bağla
        glActiveTexture(GL_TEXTURE0 + RenderConstants::CSM_SHADOW_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D_ARRAY, ctx.shadowFBO.get_depth_texture_array());
        if (u.u_ShadowMapArray >= 0) glUniform1i(u.u_ShadowMapArray, RenderConstants::CSM_SHADOW_TEXTURE_SLOT);

        // PS1 Retro Horror Parametreleri
        const auto& ps1 = ctx.renderSettings;
        const float snapVal = ps1.enable_ps1_mode ? ps1.ps1_vertex_snapping : 0.0f;
        const float depthVal = ps1.enable_ps1_mode ? ps1.ps1_color_depth : 0.0f;
        const bool ditherVal = ps1.enable_ps1_mode && ps1.ps1_dithering;
        const float ditherStr = ps1.ps1_dither_strength;
        const bool affineVal = ps1.enable_ps1_mode && ps1.ps1_affine_texture_mapping;

        if (u.u_PS1VertexSnapping >= 0) glUniform1f(u.u_PS1VertexSnapping, snapVal);
        if (u.u_PS1AffineMapping >= 0) glUniform1i(u.u_PS1AffineMapping, affineVal ? 1 : 0);
        if (u.u_PS1ColorDepth >= 0) glUniform1f(u.u_PS1ColorDepth, depthVal);
        if (u.u_PS1Dithering >= 0) glUniform1i(u.u_PS1Dithering, ditherVal ? 1 : 0);
        if (u.u_PS1DitherStrength >= 0) glUniform1f(u.u_PS1DitherStrength, ditherStr);

        const bool csmActive = ctx.renderSettings.enable_shadows && ctx.renderSettings.enable_csm && !m_CSMCascades.empty();
        if (u.u_EnableCSM >= 0) glUniform1i(u.u_EnableCSM, csmActive ? 1 : 0);
        if (csmActive)
        {
            if (u.u_CascadeCount >= 0) glUniform1i(u.u_CascadeCount, static_cast<int>(m_CSMCascades.size()));
            if (u.u_CascadePlaneDistances >= 0)
            {
                float dists[3] = { m_CSMCascades[0].split_distance, m_CSMCascades[1].split_distance, m_CSMCascades[2].split_distance };
                glUniform1fv(u.u_CascadePlaneDistances, 3, dists);
            }
            if (u.u_CascadeLightSpaceMatrices >= 0)
            {
                glm::mat4 matrices[3] = { m_CSMCascades[0].light_space_matrix, m_CSMCascades[1].light_space_matrix, m_CSMCascades[2].light_space_matrix };
                glUniformMatrix4fv(u.u_CascadeLightSpaceMatrices, 3, GL_FALSE, &matrices[0][0][0]);
            }
        }

        // Paket (Batch) içindeki varlıkların bireysel (Per-Object) çizimi (Önbellekli lokasyonlar)
        for (const auto& item : batch)
        {
            if (u.u_model >= 0) glUniformMatrix4fv(u.u_model, 1, GL_FALSE, &item.model[0][0]);
            if (u.u_baseColor >= 0) glUniform3fv(u.u_baseColor, 1, &item.base_color[0]);
            if (u.u_TextureScale >= 0) glUniform2fv(u.u_TextureScale, 1, &item.texture_scale[0]);

            // Modern PBR Parametreleri
            if (u.u_Metallic >= 0) glUniform1f(u.u_Metallic, item.metallic);
            if (u.u_Roughness >= 0) glUniform1f(u.u_Roughness, item.roughness);
            if (u.u_AO >= 0) glUniform1f(u.u_AO, item.ao);
            if (u.u_EmissiveColor >= 0) glUniform3fv(u.u_EmissiveColor, 1, &item.emissive_color[0]);
            if (u.u_EmissiveIntensity >= 0) glUniform1f(u.u_EmissiveIntensity, item.emissive_intensity);

            // Doku Slot ve Bayrak Ayarları
            if (u.u_useTexture >= 0) glUniform1i(u.u_useTexture, item.diffuse_handle.is_valid());
            if (u.u_diffuseTex >= 0) glUniform1i(u.u_diffuseTex, RenderConstants::DIFFUSE_TEXTURE_SLOT);

            if (u.u_useNormalMap >= 0) glUniform1i(u.u_useNormalMap, item.normal_handle.is_valid());
            if (u.u_normalTex >= 0) glUniform1i(u.u_normalTex, RenderConstants::NORMAL_TEXTURE_SLOT);
            if (item.normal_handle.is_valid() && u.u_normalStrength >= 0)
            {
                glUniform1f(u.u_normalStrength, item.normal_strength);
            }

            if (u.u_UseMetallicMap >= 0) glUniform1i(u.u_UseMetallicMap, item.metallic_handle.is_valid());
            if (u.u_metallicTex >= 0) glUniform1i(u.u_metallicTex, RenderConstants::METALLIC_TEXTURE_SLOT);

            if (u.u_UseRoughnessMap >= 0) glUniform1i(u.u_UseRoughnessMap, item.roughness_handle.is_valid());
            if (u.u_roughnessTex >= 0) glUniform1i(u.u_roughnessTex, RenderConstants::ROUGHNESS_TEXTURE_SLOT);

            if (u.u_UseAOMap >= 0) glUniform1i(u.u_UseAOMap, item.ao_handle.is_valid());
            if (u.u_aoTex >= 0) glUniform1i(u.u_aoTex, RenderConstants::AO_TEXTURE_SLOT);

            if (u.u_UseEmissiveMap >= 0) glUniform1i(u.u_UseEmissiveMap, item.emissive_handle.is_valid());
            if (u.u_emissiveTex >= 0) glUniform1i(u.u_emissiveTex, RenderConstants::EMISSIVE_TEXTURE_SLOT);

            // Geriye dönük uyumluluk
            if (u.u_specularStrength >= 0) glUniform1f(u.u_specularStrength, item.specular_strength);
            if (u.u_shininess >= 0) glUniform1f(u.u_shininess, item.shininess);

            if (u.u_RenderMode >= 0) glUniform1i(u.u_RenderMode, static_cast<int>(item.mode));
            if (u.u_AlphaThreshold >= 0) glUniform1f(u.u_AlphaThreshold, item.alpha_threshold);

            const unsigned int drawnIndices = ctx.renderer.Draw(item, ctx.aria_system);
            if (drawnIndices > 0)
            {
                ++passStats.draw_calls_main;
                passStats.triangles_main += drawnIndices / 3;
            }
        }

        // Program bitiminde gölge dokusu yuvalarını temizle
        glActiveTexture(GL_TEXTURE0 + RenderConstants::CSM_SHADOW_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glActiveTexture(GL_TEXTURE0 + RenderConstants::SHADOW_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }

    if (hasSkybox)
    {
        glActiveTexture(GL_TEXTURE0 + RenderConstants::SKYBOX_TEXTURE_SLOT);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glActiveTexture(GL_TEXTURE0);
    }

    glUseProgram(0);
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

    // 1. Statik Mesh Varlıklarının Çıkarımı (Modern EnTT Çoklu Bileşen View ile Bitişik Bellek Hızı)
    auto meshView = reg.view<const transform_component, const mesh_component>();
    for (auto ent : meshView)
    {
        const auto& transform = meshView.get<const transform_component>(ent);
        const auto& meshComp = meshView.get<const mesh_component>(ent);
        const entity e = to_entity(ent);
        const auto* matComp = reg.get_component<material_component>(e);

        // Model Matrisinin Hesaplanması
        const glm::mat4 model = ecs_transform::get_world_matrix(e, reg);

        // 2. Frustum Culling (Görüş Alanı Kırpması)
        const aria::submesh_runtime_info meshInfo =
            ctx.aria_system->get_submesh_info(
                meshComp.model_handle,
                meshComp.sub_mesh_index);

        const glm::vec3 localCenter = meshInfo.local_center;
        const glm::vec4 homogeneousCenter = model * glm::vec4(localCenter, 1.0f);
        const glm::vec3 worldCenter = glm::vec3(homogeneousCenter) / homogeneousCenter.w;

        const glm::vec3 absoluteScale = glm::abs(transform.scale);
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

    // 4. İskeletli (Skinned) Mesh Varlıklarının Çıkarımı (Modern EnTT View)
    auto skinnedView = reg.view<const transform_component, const skinned_mesh_component>();
    for (auto ent : skinnedView)
    {
        const auto& transform = skinnedView.get<const transform_component>(ent);
        const auto& skinnedMeshComp = skinnedView.get<const skinned_mesh_component>(ent);
        const entity e = to_entity(ent);
        const auto* matComp = reg.get_component<material_component>(e);
        const auto* skelComp = reg.get_component<skeleton_component>(e);

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

            const glm::vec3 absoluteScale = glm::abs(transform.scale);
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

    const glm::vec3 lightDir = (glm::length(light.direction) > 0.001f) ? glm::normalize(light.direction) : glm::vec3(0.0f, -1.0f, -0.5f);
    const glm::vec3 up = (std::abs(lightDir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    const glm::vec3 shadowCenter = glm::vec3(viewPos.x, 0.0f, viewPos.z);
    const glm::vec3 lightPos = shadowCenter - lightDir * ctx.renderSettings.shadow_distance;
    const glm::vec3 target = shadowCenter;

    const glm::mat4 lightView = glm::lookAt(lightPos, target, up);
    return lightProjection * lightView;
}

std::vector<csm_cascade_data> render_system::calculate_csm_cascades(
    const EngineContext& ctx,
    const light_data& light,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition)
{
    std::vector<csm_cascade_data> cascades;
    cascades.reserve(3);

    const float splits[3] = {
        ctx.renderSettings.csm_split_distances[0],
        ctx.renderSettings.csm_split_distances[1],
        ctx.renderSettings.csm_split_distances[2]
    };

    const glm::mat4 invView = glm::inverse(view);
    const glm::vec3 lightDir = (glm::length(light.direction) > 0.001f) ? glm::normalize(light.direction) : glm::vec3(0.0f, -1.0f, -0.5f);
    const glm::vec3 up = (std::abs(lightDir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    const float tanHalfFov = (std::abs(projection[1][1]) > 0.0001f) ? (1.0f / projection[1][1]) : 1.0f;
    const float aspect = (std::abs(projection[0][0]) > 0.0001f) ? (projection[1][1] / projection[0][0]) : (16.0f / 9.0f);

    for (int i = 0; i < 3; ++i)
    {
        csm_cascade_data cData;
        cData.split_distance = splits[i];

        const float prevSplit = (i == 0) ? 0.1f : splits[i - 1];
        const float curSplit = splits[i];

        const float xn = prevSplit * tanHalfFov * aspect;
        const float yn = prevSplit * tanHalfFov;
        const float xf = curSplit * tanHalfFov * aspect;
        const float yf = curSplit * tanHalfFov;

        const glm::vec4 frustumCornersView[8] = {
            glm::vec4( xn,  yn, -prevSplit, 1.0f),
            glm::vec4(-xn,  yn, -prevSplit, 1.0f),
            glm::vec4( xn, -yn, -prevSplit, 1.0f),
            glm::vec4(-xn, -yn, -prevSplit, 1.0f),
            glm::vec4( xf,  yf, -curSplit,  1.0f),
            glm::vec4(-xf,  yf, -curSplit,  1.0f),
            glm::vec4( xf, -yf, -curSplit,  1.0f),
            glm::vec4(-xf, -yf, -curSplit,  1.0f)
        };

        glm::vec3 center(0.0f);
        glm::vec3 cornersWorld[8];
        for (int k = 0; k < 8; ++k)
        {
            glm::vec4 worldPt = invView * frustumCornersView[k];
            cornersWorld[k] = glm::vec3(worldPt);
            center += cornersWorld[k];
        }
        center /= 8.0f;

        // Bounding sphere yarıçapı (kamera döndükçe gölgenin titremesini önleyen kararlı küre)
        float radius = 0.0f;
        for (int k = 0; k < 8; ++k)
        {
            radius = std::max(radius, glm::length(cornersWorld[k] - center));
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        // Işık kamerası konumu: center noktasından geriye doğru
        const float lightDistance = radius + 100.0f;
        const glm::vec3 eye = center - lightDir * lightDistance;
        const glm::mat4 lightView = glm::lookAt(eye, center, up);

        // Texel stabilizasyonu (Shadow edge shimmering engelleme)
        const float shadowRes = static_cast<float>(ctx.renderSettings.shadow_map_resolution);
        const float worldUnitsPerTexel = (2.0f * radius) / shadowRes;

        float minX = -radius;
        float maxX = radius;
        float minY = -radius;
        float maxY = radius;

        if (worldUnitsPerTexel > 0.0001f)
        {
            minX = std::floor(minX / worldUnitsPerTexel) * worldUnitsPerTexel;
            maxX = minX + 2.0f * radius;
            minY = std::floor(minY / worldUnitsPerTexel) * worldUnitsPerTexel;
            maxY = minY + 2.0f * radius;
        }

        // Pozitif zNear ve zFar (Kameranın baktığı yöndeki pozitif mesafeler - OpenGL klip uzayı [-1, 1] için zorunlu)
        const float zNear = 1.0f;
        const float zFar = 2.0f * radius + 150.0f;

        const glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, zNear, zFar);
        cData.light_space_matrix = lightProj * lightView;

        cascades.push_back(cData);
    }

    return cascades;
}
