/**
 * @file volumetric_pass.cpp
 * @brief Hacimsel ışıklandırma geçişi, FBO yönetimi, ışın yürütme ve eklemeli harmanlama uygulaması.
 */

#include "volumetric_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Core/Logger/logger.h"
#include "../render_target.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <iostream>

namespace {

    unsigned int s_quadVAO = 0;
    unsigned int s_quadVBO = 0;
    RenderTarget s_volumetricTarget;

    void initQuad()
    {
        if (s_quadVAO != 0) {
            return;
        }

        static const float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &s_quadVAO);
        glGenBuffers(1, &s_quadVBO);

        glBindVertexArray(s_quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void drawQuad()
    {
        if (s_quadVAO == 0) {
            initQuad();
        }
        glBindVertexArray(s_quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

} // anonymous namespace

void volumetric_pass::render(
    EngineContext& ctx,
    RenderTarget& target,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition,
    bool isSceneViewport
)
{
    if (!ctx.renderSettings.enable_volumetric_lighting)
    {
        return;
    }

    if (!ctx.renderSystem.has_directional_light())
    {
        return;
    }

    if (!ctx.aria_system || target.depth_tex == 0)
    {
        return;
    }

    try
    {
        // 1. Aria üzerinden shader tanıtıcılarını çözümle
        unsigned int volShaderID = ctx.aria_system->get_shader_id(ctx.volumetricShaderHandle);
        if (volShaderID == 0)
        {
            ctx.volumetricShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/Volumetric/volumetric");
            volShaderID = ctx.aria_system->get_shader_id(ctx.volumetricShaderHandle);
            if (volShaderID == 0)
            {
                QLOG_WARN(renderer, "[VolumetricPass] Hacimsel raymarch shader yuklenemedi!");
                return;
            }
        }

        unsigned int compShaderID = ctx.aria_system->get_shader_id(ctx.volumetricCompositeShaderHandle);
        if (compShaderID == 0)
        {
            ctx.volumetricCompositeShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/Volumetric/volumetric_composite");
            compShaderID = ctx.aria_system->get_shader_id(ctx.volumetricCompositeShaderHandle);
            if (compShaderID == 0)
            {
                QLOG_WARN(renderer, "[VolumetricPass] Hacimsel composite shader yuklenemedi!");
                return;
            }
        }

        // 2. Hacimsel çizim tamponu çözünürlüğünü belirle
        // Varsayılan PBR modunda performans ve pürüzsüz süzmeler için 1/2 çözünürlük kullanılır.
        // PS1 retro modunda pikselleştirme hissini korumak için 1:1 çözünürlük kullanılır.
        int volW = std::max(1, target.width / 2);
        int volH = std::max(1, target.height / 2);
        if (ctx.renderSettings.enable_ps1_mode)
        {
            volW = target.width;
            volH = target.height;
        }

        if (s_volumetricTarget.fbo == 0)
        {
            s_volumetricTarget.create(volW, volH);
        }
        else if (s_volumetricTarget.width != volW || s_volumetricTarget.height != volH)
        {
            s_volumetricTarget.resize(volW, volH);
        }

        // ====================================================================
        // GEÇİŞ 1: HACİMSEL IŞIN YÜRÜTME (VOLUMETRIC RAYMARCHING)
        // ====================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, s_volumetricTarget.fbo);
        glViewport(0, 0, s_volumetricTarget.width, s_volumetricTarget.height);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(volShaderID);

        // Dokuları bağla
        // Slot 0: Sahne derinlik dokusu
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, target.depth_tex);
        glUniform1i(glGetUniformLocation(volShaderID, "u_SceneDepth"), 0);

        // Slot 1: Standart tekli gölge haritası
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ctx.shadowFBO.get_depth_texture());
        glUniform1i(glGetUniformLocation(volShaderID, "u_ShadowMap"), 1);

        // Slot 2: Kaskatlı gölge haritası dizisi (CSM)
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D_ARRAY, ctx.shadowFBO.get_depth_texture_array());
        glUniform1i(glGetUniformLocation(volShaderID, "u_ShadowMapArray"), 2);

        // Kamera & Projeksiyon uniformları
        const glm::mat4 viewProj = projection * view;
        const glm::mat4 invViewProj = glm::inverse(viewProj);

        glUniform3fv(glGetUniformLocation(volShaderID, "u_CameraPos"), 1, &cameraPosition[0]);
        glUniformMatrix4fv(glGetUniformLocation(volShaderID, "u_InvViewProj"), 1, GL_FALSE, &invViewProj[0][0]);

        // Işık bilgileri
        const auto& dirLight = ctx.renderSystem.get_directional_light();
        glUniform3fv(glGetUniformLocation(volShaderID, "u_LightDir"), 1, &dirLight.direction[0]);
        glUniform3fv(glGetUniformLocation(volShaderID, "u_LightColor"), 1, &dirLight.color[0]);
        glUniform1f(glGetUniformLocation(volShaderID, "u_LightIntensity"), dirLight.intensity);

        // Hacimsel parametreler (Sis aktifse yoğunluk tabanını sis yoğunluğu ile senkronize destekle)
        const bool fogActive = ctx.fog.enabled && (isSceneViewport ? ctx.fog.show_in_scene : true);
        glUniform1f(glGetUniformLocation(volShaderID, "u_VolumetricIntensity"), ctx.renderSettings.volumetric_intensity);
        glUniform1i(glGetUniformLocation(volShaderID, "u_Steps"), ctx.renderSettings.volumetric_steps);
        glUniform1f(glGetUniformLocation(volShaderID, "u_ScatteringG"), ctx.renderSettings.volumetric_scattering);
        glUniform1f(glGetUniformLocation(volShaderID, "u_Density"), ctx.renderSettings.volumetric_density);
        glUniform1f(glGetUniformLocation(volShaderID, "u_MaxDistance"), ctx.renderSettings.volumetric_max_distance);
        glUniform1i(glGetUniformLocation(volShaderID, "u_FogEnabled"), fogActive ? 1 : 0);
        glUniform1i(glGetUniformLocation(volShaderID, "u_FogMode"), ctx.fog.mode);
        glUniform1f(glGetUniformLocation(volShaderID, "u_FogHeightFalloff"), ctx.fog.height_falloff);
        glUniform1f(glGetUniformLocation(volShaderID, "u_FogBaseHeight"), ctx.fog.base_height);

        // Gölge uniformları
        const auto& csmCascades = ctx.renderSystem.get_csm_cascades();
        const bool csmActive = ctx.renderSettings.enable_shadows && ctx.renderSettings.enable_csm && !csmCascades.empty();

        glUniform1i(glGetUniformLocation(volShaderID, "u_EnableShadows"), ctx.renderSettings.enable_shadows ? 1 : 0);
        glUniform1i(glGetUniformLocation(volShaderID, "u_EnableCSM"), csmActive ? 1 : 0);

        if (csmActive)
        {
            glUniform1i(glGetUniformLocation(volShaderID, "u_CascadeCount"), static_cast<int>(csmCascades.size()));
            float dists[3] = { csmCascades[0].split_distance, csmCascades[1].split_distance, csmCascades[2].split_distance };
            glUniform1fv(glGetUniformLocation(volShaderID, "u_CascadePlaneDistances"), 3, dists);
            glm::mat4 matrices[3] = { csmCascades[0].light_space_matrix, csmCascades[1].light_space_matrix, csmCascades[2].light_space_matrix };
            glUniformMatrix4fv(glGetUniformLocation(volShaderID, "u_CascadeLightSpaceMatrices"), 3, GL_FALSE, &matrices[0][0][0]);
        }
        else
        {
            const glm::mat4& lsm = ctx.renderSystem.get_light_space_matrix();
            glUniformMatrix4fv(glGetUniformLocation(volShaderID, "u_LightSpaceMatrix"), 1, GL_FALSE, &lsm[0][0]);
        }

        // PS1 Retro Horror Parametreleri
        glUniform1i(glGetUniformLocation(volShaderID, "u_PS1Mode"), ctx.renderSettings.enable_ps1_mode ? 1 : 0);
        glUniform1i(glGetUniformLocation(volShaderID, "u_PS1Dithering"), ctx.renderSettings.ps1_dithering ? 1 : 0);
        glUniform1f(glGetUniformLocation(volShaderID, "u_PS1DitherStrength"), ctx.renderSettings.ps1_dither_strength);
        glUniform1f(glGetUniformLocation(volShaderID, "u_PS1ColorDepth"), ctx.renderSettings.ps1_color_depth);

        // Quad çizimi ile ışın yürütmeyi gerçekleştir
        drawQuad();

        // ====================================================================
        // GEÇİŞ 2: EKLEMELİ BİRLEŞTİRME (ADDITIVE COMPOSITE)
        // ====================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
        glViewport(0, 0, target.width, target.height);

        glEnable(GL_BLEND);
        // RGB için eklemeli (additive) harmanlama, Alpha değerini koru
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        glUseProgram(compShaderID);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_volumetricTarget.tex);

        if (ctx.renderSettings.enable_ps1_mode)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        glUniform1i(glGetUniformLocation(compShaderID, "u_VolumetricTexture"), 0);

        // Additive olarak sahneye çiz
        drawQuad();

        // ====================================================================
        // OPENGL DURUMLARINI GERİ YÜKLE
        // ====================================================================
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    catch (const std::exception& e)
    {
        QLOG_ERROR(renderer, "[VolumetricPass] Kritik render hatasi: {}", e.what());
    }
    catch (...)
    {
        QLOG_ERROR(renderer, "[VolumetricPass] Bilinmeyen kritik render hatasi!");
    }
}

void volumetric_pass::shutdown() noexcept
{
    s_volumetricTarget.destroy();

    if (s_quadVBO != 0)
    {
        glDeleteBuffers(1, &s_quadVBO);
        s_quadVBO = 0;
    }

    if (s_quadVAO != 0)
    {
        glDeleteVertexArrays(1, &s_quadVAO);
        s_quadVAO = 0;
    }
}
