/**
 * @file editor_pass.cpp
 * @brief Editör sahne çizim geçişi ve FBO yönetimi uygulaması.
 */

#include "editor_pass.h"
#include "../../Engine/engine_context.h"
#include "../../Editor/editor_context.h"
#include "../../Editor/editor_camera.h"
#include "skybox_pass.h"
#include "ssao_pass.h"
#include "volumetric_pass.h"
#include "post_process_pass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

void editor_pass::render(EngineContext& ctx, EditorContext& editorCtx)
{
    if (!editorCtx.camera) {
        return;
    }

    // 1. Hedef Framebuffer ve Viewport yapılandırması
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.editorTarget.fbo);
    glViewport(0, 0, ctx.editorTarget.width, ctx.editorTarget.height);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(
        ctx.renderSettings.clear_color_scene.r,
        ctx.renderSettings.clear_color_scene.g,
        ctx.renderSettings.clear_color_scene.b,
        0.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Kamera matrislerini hesapla (sıfıra bölme koruması ile)
    const float aspect = static_cast<float>(ctx.editorTarget.width) /
        static_cast<float>(std::max(1, ctx.editorTarget.height));

    const glm::mat4 editorView = editorCtx.camera->getViewMatrix();
    const glm::mat4 editorProj = editorCtx.camera->getProjectionMatrix(aspect);
    const glm::mat4 jitteredProj = post_process_pass::apply_taa_jitter(
        ctx, editorProj, ctx.editorTarget.width, ctx.editorTarget.height, true
    );

    // 3. Gökyüzü (Skybox) render geçişi
    skybox_pass::render(ctx, editorView, jitteredProj);

    // 4. Ana sahne geometrisi ve ışıklandırma (isSceneViewport = true)
    ctx.renderSystem.render(
        ctx.session.editor_scene().get_registry(),
        ctx.session.editor_scene().get_light_batch(),
        ctx,
        editorView,
        jitteredProj,
        true
    );

    // 4.5. Ekran Uzayı Ortam Örtmesi (SSAO/GTAO) ve Küresel Aydınlatma (SSGI)
    // Jitter'sız projeksiyon matrisi kullanılarak gölge/AO/SSGI kararsızlığı engellenir.
    ssao_pass::render(ctx, ctx.editorTarget, editorView, editorProj, true);

    // 5. Hacimsel Işıklandırma (Volumetric Lighting / God Rays)
    volumetric_pass::render(ctx, ctx.editorTarget, editorView, editorProj, editorCtx.camera->position, true);

    // 6. HDR Ton Eşleme (Tone Mapping), sRGB Dönüşümü ve Kenar Yumuşatma (FXAA / TAA)
    post_process_pass::render(ctx, ctx.editorTarget, editorView, editorProj, true);
}
