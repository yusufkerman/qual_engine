/**
 * @file post_process_pass.cpp
 * @brief HDR Tone Mapping, sRGB Gamma Düzeltmesi ve FXAA 3.11 / TAA (Halton Jitter + Variance Clipping) uygulaması.
 */

#include "post_process_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Engine/asset_pipeline.h"
#include "../../Core/Logger/logger.h"
#include "../render_target.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>

namespace {

    unsigned int s_quadVAO = 0;
    unsigned int s_quadVBO = 0;

    RenderTarget s_hdrScratchTarget;

    struct ViewportTAAState
    {
        RenderTarget historyTargets[2];
        int writeIndex = 0;
        uint32_t frameIndex = 0;
        bool hasHistory = false;
        glm::mat4 prevViewProj{ 1.0f };
    };

    ViewportTAAState s_editorTAA;
    ViewportTAAState s_gameTAA;

    float haltonSequence(int index, int base) noexcept
    {
        float result = 0.0f;
        float f = 1.0f / static_cast<float>(base);
        int i = index;
        while (i > 0)
        {
            result += f * static_cast<float>(i % base);
            i /= base;
            f /= static_cast<float>(base);
        }
        return result;
    }

    void initQuad()
    {
        if (s_quadVAO != 0) {
            return;
        }

        static const float quadVertices[] = {
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

glm::mat4 post_process_pass::apply_taa_jitter(
    const EngineContext& ctx,
    const glm::mat4& unjitteredProj,
    int width,
    int height,
    bool isSceneViewport
) noexcept
{
    if (ctx.renderSettings.enable_ps1_mode || ctx.renderSettings.aa_mode != 2)
    {
        return unjitteredProj;
    }

    const int w = std::max(1, width);
    const int h = std::max(1, height);

    ViewportTAAState& taa = isSceneViewport ? s_editorTAA : s_gameTAA;
    const int sampleIdx = static_cast<int>(taa.frameIndex % 8u) + 1;

    // Halton (2, 3) [-0.5, +0.5] piksel aralığında alt-piksel kaydırma
    const float jitterX = (haltonSequence(sampleIdx, 2) - 0.5f) * (2.0f / static_cast<float>(w));
    const float jitterY = (haltonSequence(sampleIdx, 3) - 0.5f) * (2.0f / static_cast<float>(h));

    glm::mat4 jittered = unjitteredProj;
    jittered[2][0] += jitterX;
    jittered[2][1] += jitterY;
    return jittered;
}

void post_process_pass::render(
    EngineContext& ctx,
    RenderTarget& target,
    const glm::mat4& view,
    const glm::mat4& unjitteredProj,
    bool isSceneViewport
)
{
    if (!ctx.aria_system || target.fbo == 0 || target.tex == 0)
    {
        return;
    }

    try
    {
        unsigned int ppShaderID = ctx.aria_system->get_shader_id(ctx.postProcessShaderHandle);
        if (ppShaderID == 0)
        {
            ctx.postProcessShaderHandle = asset_pipeline::import_shader(ctx, "@engine/Shaders/PostProcess/post_process");
            ppShaderID = ctx.aria_system->get_shader_id(ctx.postProcessShaderHandle);
            if (ppShaderID == 0)
            {
                QLOG_WARN(renderer, "[PostProcessPass] post_process shader yuklenemedi!");
                return;
            }
        }

        const int w = std::max(1, target.width);
        const int h = std::max(1, target.height);

        if (s_hdrScratchTarget.fbo == 0)
        {
            s_hdrScratchTarget.create(w, h);
        }
        else if (s_hdrScratchTarget.width != w || s_hdrScratchTarget.height != h)
        {
            s_hdrScratchTarget.resize(w, h);
        }

        // 1. Mevcut HDR hedef tamponunu (target.fbo) s_hdrScratchTarget'a kopyala
        glBindFramebuffer(GL_READ_FRAMEBUFFER, target.fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_hdrScratchTarget.fbo);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);

        glUseProgram(ppShaderID);

        const float texelX = 1.0f / static_cast<float>(w);
        const float texelY = 1.0f / static_cast<float>(h);
        glUniform2f(glGetUniformLocation(ppShaderID, "u_TexelSize"), texelX, texelY);

        const bool ps1Mode = ctx.renderSettings.enable_ps1_mode;
        const int aaMode = ps1Mode ? 0 : ctx.renderSettings.aa_mode;

        ViewportTAAState& taa = isSceneViewport ? s_editorTAA : s_gameTAA;
        const glm::mat4 currentViewProj = unjitteredProj * view;

        unsigned int toneMapSourceTex = s_hdrScratchTarget.tex;

        // ====================================================================
        // AŞAMA 1: TAA ZAMAN İÇİ BİRLEŞTİRME (Yalnızca aa_mode == 2 ise)
        // ====================================================================
        if (aaMode == 2 && target.depth_tex != 0)
        {
            for (int i = 0; i < 2; ++i)
            {
                if (taa.historyTargets[i].fbo == 0)
                {
                    taa.historyTargets[i].create(w, h);
                    taa.hasHistory = false;
                }
                else if (taa.historyTargets[i].width != w || taa.historyTargets[i].height != h)
                {
                    taa.historyTargets[i].resize(w, h);
                    taa.hasHistory = false;
                }
            }

            const int readIdx = 1 - taa.writeIndex;
            const int writeIdx = taa.writeIndex;

            glBindFramebuffer(GL_FRAMEBUFFER, taa.historyTargets[writeIdx].fbo);
            glViewport(0, 0, w, h);

            glUniform1i(glGetUniformLocation(ppShaderID, "u_PassMode"), 0);
            glUniform1i(glGetUniformLocation(ppShaderID, "u_HasHistory"), taa.hasHistory ? 1 : 0);
            glUniform1f(glGetUniformLocation(ppShaderID, "u_TAABlend"), ctx.renderSettings.taa_blend);

            const glm::mat4 invViewProj = glm::inverse(currentViewProj);
            glUniformMatrix4fv(glGetUniformLocation(ppShaderID, "u_InvViewProj"), 1, GL_FALSE, glm::value_ptr(invViewProj));
            glUniformMatrix4fv(glGetUniformLocation(ppShaderID, "u_PrevViewProj"), 1, GL_FALSE, glm::value_ptr(taa.prevViewProj));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, s_hdrScratchTarget.tex);
            glUniform1i(glGetUniformLocation(ppShaderID, "u_SceneTexture"), 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, taa.historyTargets[readIdx].tex);
            glUniform1i(glGetUniformLocation(ppShaderID, "u_HistoryTexture"), 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, target.depth_tex);
            glUniform1i(glGetUniformLocation(ppShaderID, "u_SceneDepth"), 2);

            drawQuad();

            toneMapSourceTex = taa.historyTargets[writeIdx].tex;
            taa.prevViewProj = currentViewProj;
            taa.hasHistory = true;
            taa.writeIndex = 1 - taa.writeIndex;
            ++taa.frameIndex;
        }
        else
        {
            taa.hasHistory = false;
            taa.prevViewProj = currentViewProj;
        }

        // ====================================================================
        // AŞAMA 2: HDR TONE MAPPING + sRGB GAMMA + FXAA 3.11 / TAA SHARPEN
        // ====================================================================
        glBindFramebuffer(GL_FRAMEBUFFER, target.fbo);
        glViewport(0, 0, w, h);

        glUniform1i(glGetUniformLocation(ppShaderID, "u_PassMode"), 1);
        glUniform1i(glGetUniformLocation(ppShaderID, "u_ToneMapOperator"), ctx.renderSettings.pbr_tone_mapping);
        glUniform1f(glGetUniformLocation(ppShaderID, "u_ExposureEV"), ctx.renderSettings.pbr_exposure);
        glUniform1f(glGetUniformLocation(ppShaderID, "u_WhitePoint"), ctx.renderSettings.pbr_white_point);
        glUniform1i(glGetUniformLocation(ppShaderID, "u_PS1Mode"), ps1Mode ? 1 : 0);
        glUniform1i(glGetUniformLocation(ppShaderID, "u_AAMode"), aaMode);
        glUniform1f(glGetUniformLocation(ppShaderID, "u_TAASharpen"), ctx.renderSettings.taa_sharpen);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, toneMapSourceTex);
        glUniform1i(glGetUniformLocation(ppShaderID, "u_SceneTexture"), 0);

        drawQuad();

        // PS1 retro modunda hedef dokunun GL_NEAREST, standart modda GL_LINEAR filtrelenmesini sağla
        glBindTexture(GL_TEXTURE_2D, target.tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ps1Mode ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ps1Mode ? GL_NEAREST : GL_LINEAR);

        // OpenGL durumlarını geri yükle
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glUseProgram(0);
    }
    catch (const std::exception& e)
    {
        QLOG_ERROR(renderer, "[PostProcessPass] Kritik render hatasi: {}", e.what());
    }
    catch (...)
    {
        QLOG_ERROR(renderer, "[PostProcessPass] Bilinmeyen kritik render hatasi!");
    }
}

void post_process_pass::shutdown() noexcept
{
    s_hdrScratchTarget.destroy();
    for (int i = 0; i < 2; ++i)
    {
        s_editorTAA.historyTargets[i].destroy();
        s_gameTAA.historyTargets[i].destroy();
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
}
