/**
 * @file skybox_pass.cpp
 * @brief Gökyüzü kutusu geometri tamponu üretimi, derinlik durumu yönetimi ve küp haritası çizim uygulaması.
 */

#include "skybox_pass.h"
#include "../../Engine/engine_context.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace {

    unsigned int s_skyVAO = 0;
    unsigned int s_skyVBO = 0;

    constexpr float s_skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    /**
     * @brief Skybox küp geometrisi için tepe noktası tamponlarını oluşturur.
     */
    void initSkybox()
    {
        glGenVertexArrays(1, &s_skyVAO);
        glGenBuffers(1, &s_skyVBO);

        glBindVertexArray(s_skyVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_skyVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(s_skyboxVertices), s_skyboxVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

} // anonymous namespace

void skybox_pass::render(EngineContext& ctx, const glm::mat4& view, const glm::mat4& projection)
{
    if (!ctx.aria_system || ctx.skyboxTexture == 0) {
        return;
    }

    const unsigned int skyboxGLShaderID = ctx.aria_system->get_shader_id(ctx.skyboxShaderHandle);
    if (skyboxGLShaderID == 0) {
        return;
    }

    if (s_skyVAO == 0) {
        initSkybox();
    }

    // Skybox en uzakta çizileceği için derinlik testini LEQUAL yap ve derinlik yazımını kapat
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glUseProgram(skyboxGLShaderID);

    // Kameranın dünya konumunu (ötelemesini) sıfırla, yalnızca dönüşü koru
    const glm::mat4 skyView = glm::mat4(glm::mat3(view));

    const GLint skyboxLoc = glGetUniformLocation(skyboxGLShaderID, "skybox");
    if (skyboxLoc != -1) {
        glUniform1i(skyboxLoc, 0);
    }

    const GLint viewLoc = glGetUniformLocation(skyboxGLShaderID, "view");
    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(skyView));
    }

    const GLint projLoc = glGetUniformLocation(skyboxGLShaderID, "projection");
    if (projLoc != -1) {
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }

    // Küp haritası dokusunu bağla ve geometriyi çiz
    glBindVertexArray(s_skyVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.skyboxTexture);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    // OpenGL durumlarını varsayılana geri yükle
    glBindVertexArray(0);
    glUseProgram(0);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void skybox_pass::shutdown()
{
    if (s_skyVBO != 0) {
        glDeleteBuffers(1, &s_skyVBO);
        s_skyVBO = 0;
    }

    if (s_skyVAO != 0) {
        glDeleteVertexArrays(1, &s_skyVAO);
        s_skyVAO = 0;
    }
}