/**
 * @file ssao_pass.cpp
 * @brief Ekran Uzayı Ortam Örtmesi (SSAO / GTAO) ve Ekran Uzayı Küresel Aydınlatma (SSGI) geçişi,
 * FBO yönetimi, derinlik koruyucu bilateral filtreleme ve dolaylı ışık harmanlama uygulaması.
 */

#include "ssao_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Core/Logger/logger.h"
#include "../render_target.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <random>
#include <vector>

namespace {

    unsigned int s_quadVAO = 0;
    unsigned int s_quadVBO = 0;
    unsigned int s_noiseTexture = 0;
    std::vector<glm::vec3> s_ssaoKernel;

    RenderTarget s_ssaoTarget;
    RenderTarget s_ssaoBlurTarget;
    RenderTarget s_ssgiTarget;
    RenderTarget s_ssgiBlurTarget;
    RenderTarget s_sceneCopyTarget;

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

    void ensureTargetSize(RenderTarget& rt, int w, int h)
    {
        if (rt.fbo == 0)
        {
            rt.create(w, h);
        }
        else if (rt.width != w || rt.height != h)
        {
            rt.resize(w, h);
        }
    }

    void initKernelAndNoise()
    {
        if (!s_ssaoKernel.empty() && s_noiseTexture != 0) {
            return;
        }

        // 1. Deterministik örneklem çekirdeği (32 adet yarım küre numunesi)
        std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
        std::default_random_engine generator(1337);

        s_ssaoKernel.clear();
        s_ssaoKernel.reserve(32);
        for (unsigned int i = 0; i < 32; ++i)
        {
            glm::vec3 sample(
                randomFloats(generator) * 2.0f - 1.0f,
                randomFloats(generator) * 2.0f - 1.0f,
                randomFloats(generator) // z in [0.0, 1.0] (hemisphere)
            );
            sample = glm::normalize(sample);
            sample *= randomFloats(generator);

            // Merkeze daha yoğun örnekleme düşmesi için kuadratik ağırlıklandırma
            float scale = static_cast<float>(i) / 32.0f;
            scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
            sample *= scale;
            s_ssaoKernel.push_back(sample);
        }

        // 2. 4x4 rastgele döndürme gürültüsü dokusu (Noise Texture)
        if (s_noiseTexture == 0)
        {
            std::vector<glm::vec3> ssaoNoise;
            ssaoNoise.reserve(16);
            for (unsigned int i = 0; i < 16; ++i)
            {
                glm::vec3 noise(
                    randomFloats(generator) * 2.0f - 1.0f,
                    randomFloats(generator) * 2.0f - 1.0f,
                    0.0f
                );
                ssaoNoise.push_back(noise);
            }

            glGenTextures(1, &s_noiseTexture);
            glBindTexture(GL_TEXTURE_2D, s_noiseTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

} // anonymous namespace

void ssao_pass::render(
    EngineContext& ctx,
    RenderTarget& target,
    const glm::mat4& view,
    const glm::mat4& projection,
    bool /*isSceneViewport*/
)
{
    const bool aoActive = ctx.renderSettings.enable_ssao;
    const bool ssgiActive = ctx.renderSettings.enable_gi &&
                            (ctx.renderSettings.gi_method == 1) &&
                            !ctx.renderSettings.enable_ps1_mode;

    if (!aoActive && !ssgiActive)
    {
        return;
    }

    if (!ctx.aria_system || target.depth_tex == 0)
    {
        return;
    }

    try
    {
        initKernelAndNoise();

        // 1. Aria üzerinden shader tanıtıcılarını çözümle
        const bool useGTAO = (ctx.renderSettings.ao_method == 1 && !ctx.renderSettings.enable_ps1_mode);
        unsigned int activeAOShaderID = 0;
        unsigned int ssaoShaderID = 0;

        if (aoActive)
        {
            if (useGTAO)
            {
                activeAOShaderID = ctx.aria_system->get_shader_id(ctx.gtaoShaderHandle);
                if (activeAOShaderID == 0)
                {
                    ctx.gtaoShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/SSAO/gtao");
                    activeAOShaderID = ctx.aria_system->get_shader_id(ctx.gtaoShaderHandle);
                }
            }

            if (!useGTAO || activeAOShaderID == 0)
            {
                ssaoShaderID = ctx.aria_system->get_shader_id(ctx.ssaoShaderHandle);
                if (ssaoShaderID == 0)
                {
                    ctx.ssaoShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/SSAO/ssao");
                    ssaoShaderID = ctx.aria_system->get_shader_id(ctx.ssaoShaderHandle);
                    if (ssaoShaderID == 0)
                    {
                        QLOG_WARN(renderer, "[SSAOPass] AO hesaplama shader yuklenemedi!");
                        return;
                    }
                }
                activeAOShaderID = ssaoShaderID;
            }
        }

        unsigned int ssgiShaderID = 0;
        if (ssgiActive)
        {
            ssgiShaderID = ctx.aria_system->get_shader_id(ctx.ssgiShaderHandle);
            if (ssgiShaderID == 0)
            {
                ctx.ssgiShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/SSAO/ssgi");
                ssgiShaderID = ctx.aria_system->get_shader_id(ctx.ssgiShaderHandle);
            }
        }

        unsigned int blurShaderID = ctx.aria_system->get_shader_id(ctx.ssaoBlurShaderHandle);
        if (blurShaderID == 0)
        {
            ctx.ssaoBlurShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/SSAO/ssao_blur");
            blurShaderID = ctx.aria_system->get_shader_id(ctx.ssaoBlurShaderHandle);
            if (blurShaderID == 0)
            {
                QLOG_WARN(renderer, "[SSAOPass] SSAO bilateral blur shader yuklenemedi!");
                return;
            }
        }

        unsigned int compShaderID = ctx.aria_system->get_shader_id(ctx.ssaoCompositeShaderHandle);
        if (compShaderID == 0)
        {
            ctx.ssaoCompositeShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/SSAO/ssao_composite");
            compShaderID = ctx.aria_system->get_shader_id(ctx.ssaoCompositeShaderHandle);
            if (compShaderID == 0)
            {
                QLOG_WARN(renderer, "[SSAOPass] SSAO composite shader yuklenemedi!");
                return;
            }
        }

        // 2. Hedef tampon çözünürlüklerini hazırla ve sahne HDR rengini kopyala
        const int w = std::max(1, target.width);
        const int h = std::max(1, target.height);

        ensureTargetSize(s_sceneCopyTarget, w, h);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, target.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_sceneCopyTarget.fbo);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);

        const glm::mat4 invProj = glm::inverse(projection);

        // ====================================================================
        // GEÇİŞ 1 & 2: ORTAM GÖLGELENDİRMESİ (SSAO / GTAO + BILATERAL BLUR)
        // ====================================================================
        if (aoActive && activeAOShaderID != 0)
        {
            ensureTargetSize(s_ssaoTarget, w, h);
            ensureTargetSize(s_ssaoBlurTarget, w, h);

            glBindFramebuffer(GL_FRAMEBUFFER, s_ssaoTarget.fbo);
            glViewport(0, 0, s_ssaoTarget.width, s_ssaoTarget.height);

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(activeAOShaderID);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, target.depth_tex);
            glUniform1i(glGetUniformLocation(activeAOShaderID, "u_SceneDepth"), 0);

            glUniformMatrix4fv(glGetUniformLocation(activeAOShaderID, "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(activeAOShaderID, "u_InvProjection"), 1, GL_FALSE, glm::value_ptr(invProj));
            glUniform2f(glGetUniformLocation(activeAOShaderID, "u_ScreenSize"), static_cast<float>(s_ssaoTarget.width), static_cast<float>(s_ssaoTarget.height));

            if (useGTAO && activeAOShaderID != ssaoShaderID)
            {
                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_Radius"), ctx.renderSettings.ssao_radius);
                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_Falloff"), ctx.renderSettings.gtao_falloff);
                glUniform1i(glGetUniformLocation(activeAOShaderID, "u_SampleSteps"), ctx.renderSettings.gtao_steps);
                glUniform1i(glGetUniformLocation(activeAOShaderID, "u_Directions"), ctx.renderSettings.gtao_directions);
                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_Intensity"), ctx.renderSettings.ssao_intensity);
                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_Thickness"), ctx.renderSettings.gtao_thickness);
            }
            else
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, s_noiseTexture);
                glUniform1i(glGetUniformLocation(activeAOShaderID, "u_NoiseTexture"), 1);

                glUniform3fv(glGetUniformLocation(activeAOShaderID, "u_Samples"), static_cast<GLsizei>(s_ssaoKernel.size()), glm::value_ptr(s_ssaoKernel[0]));

                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_Radius"), ctx.renderSettings.ssao_radius);
                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_Bias"), ctx.renderSettings.ssao_bias);
                glUniform1i(glGetUniformLocation(activeAOShaderID, "u_SampleCount"), ctx.renderSettings.ssao_sample_count);
                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_Intensity"), ctx.renderSettings.ssao_intensity);

                glUniform1i(glGetUniformLocation(activeAOShaderID, "u_PS1Mode"), ctx.renderSettings.enable_ps1_mode ? 1 : 0);
                glUniform1i(glGetUniformLocation(activeAOShaderID, "u_PS1Dithering"), ctx.renderSettings.ps1_dithering ? 1 : 0);
                glUniform1f(glGetUniformLocation(activeAOShaderID, "u_PS1DitherStrength"), ctx.renderSettings.ps1_dither_strength);
            }

            drawQuad();

            // AO Bilateral Blur
            glBindFramebuffer(GL_FRAMEBUFFER, s_ssaoBlurTarget.fbo);
            glViewport(0, 0, s_ssaoBlurTarget.width, s_ssaoBlurTarget.height);

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(blurShaderID);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, s_ssaoTarget.tex);
            glUniform1i(glGetUniformLocation(blurShaderID, "u_SSAOTexture"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, target.depth_tex);
            glUniform1i(glGetUniformLocation(blurShaderID, "u_SceneDepth"), 1);

            glUniform2f(glGetUniformLocation(blurShaderID, "u_TexelSize"),
                1.0f / static_cast<float>(s_ssaoBlurTarget.width),
                1.0f / static_cast<float>(s_ssaoBlurTarget.height));
            glUniform1i(glGetUniformLocation(blurShaderID, "u_PS1Mode"), ctx.renderSettings.enable_ps1_mode ? 1 : 0);
            glUniform1i(glGetUniformLocation(blurShaderID, "u_EnableBlur"), ctx.renderSettings.ssao_blur ? 1 : 0);
            glUniform1f(glGetUniformLocation(blurShaderID, "u_BlurStride"), 1.0f);

            drawQuad();
        }

        // ====================================================================
        // GEÇİŞ 2.5: YARIM ÇÖZÜNÜRLÜKLÜ EKRAN UZAYI DOLAYLI IŞIK (SSGI + DENOISE)
        // ====================================================================
        const bool canRunSSGI = ssgiActive && (ssgiShaderID != 0);
        if (canRunSSGI)
        {
            const int giW = std::max(1, w / 2);
            const int giH = std::max(1, h / 2);
            ensureTargetSize(s_ssgiTarget, giW, giH);
            ensureTargetSize(s_ssgiBlurTarget, giW, giH);

            glBindFramebuffer(GL_FRAMEBUFFER, s_ssgiTarget.fbo);
            glViewport(0, 0, giW, giH);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(ssgiShaderID);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, target.depth_tex);
            glUniform1i(glGetUniformLocation(ssgiShaderID, "u_SceneDepth"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, s_sceneCopyTarget.tex);
            glUniform1i(glGetUniformLocation(ssgiShaderID, "u_SceneColor"), 1);

            glUniformMatrix4fv(glGetUniformLocation(ssgiShaderID, "u_Projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(ssgiShaderID, "u_InvProjection"), 1, GL_FALSE, glm::value_ptr(invProj));
            glUniform2f(glGetUniformLocation(ssgiShaderID, "u_ScreenSize"), static_cast<float>(giW), static_cast<float>(giH));
            glUniform1f(glGetUniformLocation(ssgiShaderID, "u_Radius"), ctx.renderSettings.gi_radius);
            glUniform1f(glGetUniformLocation(ssgiShaderID, "u_Intensity"), ctx.renderSettings.gi_intensity);
            glUniform1i(glGetUniformLocation(ssgiShaderID, "u_SampleCount"), ctx.renderSettings.gi_sample_count);

            drawQuad();

            // SSGI Bilateral Denoise (Yarım çözünürlükte genişletilmiş adım)
            glBindFramebuffer(GL_FRAMEBUFFER, s_ssgiBlurTarget.fbo);
            glViewport(0, 0, giW, giH);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(blurShaderID);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, s_ssgiTarget.tex);
            glUniform1i(glGetUniformLocation(blurShaderID, "u_SSAOTexture"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, target.depth_tex);
            glUniform1i(glGetUniformLocation(blurShaderID, "u_SceneDepth"), 1);

            glUniform2f(glGetUniformLocation(blurShaderID, "u_TexelSize"),
                1.0f / static_cast<float>(giW),
                1.0f / static_cast<float>(giH));
            glUniform1i(glGetUniformLocation(blurShaderID, "u_PS1Mode"), 0);
            glUniform1i(glGetUniformLocation(blurShaderID, "u_EnableBlur"), 1);
            glUniform1f(glGetUniformLocation(blurShaderID, "u_BlurStride"), 1.5f);

            drawQuad();
        }

        // ====================================================================
        // GEÇİŞ 3: YALNIZCA DOLAYLI IŞIĞA AO UYGULAMA VE SSGI HARMANLAMA
        // ====================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
        glViewport(0, 0, target.width, target.height);

        glDisable(GL_BLEND);

        glUseProgram(compShaderID);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, aoActive ? s_ssaoBlurTarget.tex : s_sceneCopyTarget.tex);
        if (aoActive)
        {
            const GLint filter = ctx.renderSettings.enable_ps1_mode ? GL_NEAREST : GL_LINEAR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, target.depth_tex);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, s_sceneCopyTarget.tex);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, canRunSSGI ? s_ssgiBlurTarget.tex : s_sceneCopyTarget.tex);

        glUniform1i(glGetUniformLocation(compShaderID, "u_SSAOTexture"), 0);
        glUniform1i(glGetUniformLocation(compShaderID, "u_SceneDepth"), 1);
        glUniform1i(glGetUniformLocation(compShaderID, "u_SceneColor"), 2);
        glUniform1i(glGetUniformLocation(compShaderID, "u_SSGITexture"), 3);

        const float compIntensity = (useGTAO && activeAOShaderID != ssaoShaderID) ? 1.0f : ctx.renderSettings.ssao_intensity;
        glUniform1f(glGetUniformLocation(compShaderID, "u_Intensity"), compIntensity);
        glUniform1i(glGetUniformLocation(compShaderID, "u_EnableAO"), aoActive ? 1 : 0);
        glUniform1i(glGetUniformLocation(compShaderID, "u_EnableSSGI"), canRunSSGI ? 1 : 0);
        glUniform1f(glGetUniformLocation(compShaderID, "u_SSGIIntensity"), ctx.renderSettings.gi_intensity);
        glUniform1i(glGetUniformLocation(compShaderID, "u_ShowDebug"), (aoActive && ctx.renderSettings.show_ssao_debug) ? 1 : 0);
        glUniform1i(glGetUniformLocation(compShaderID, "u_PS1Mode"), ctx.renderSettings.enable_ps1_mode ? 1 : 0);

        drawQuad();

        // ====================================================================
        // OPENGL DURUMLARINI GERİ YÜKLE
        // ====================================================================
        for (int slot = 3; slot >= 0; --slot)
        {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glUseProgram(0);
    }
    catch (const std::exception& e)
    {
        QLOG_ERROR(renderer, "[SSAOPass] Kritik render hatasi: {}", e.what());
    }
    catch (...)
    {
        QLOG_ERROR(renderer, "[SSAOPass] Bilinmeyen kritik render hatasi!");
    }
}

void ssao_pass::shutdown() noexcept
{
    s_ssaoTarget.destroy();
    s_ssaoBlurTarget.destroy();
    s_ssgiTarget.destroy();
    s_ssgiBlurTarget.destroy();
    s_sceneCopyTarget.destroy();

    if (s_noiseTexture != 0)
    {
        glDeleteTextures(1, &s_noiseTexture);
        s_noiseTexture = 0;
    }

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

    s_ssaoKernel.clear();
}

unsigned int ssao_pass::get_debug_texture() noexcept
{
    if (s_ssaoBlurTarget.tex != 0)
    {
        return s_ssaoBlurTarget.tex;
    }
    return s_ssaoTarget.tex;
}
