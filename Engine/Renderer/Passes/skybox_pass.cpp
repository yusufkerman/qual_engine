#include "skybox_pass.h"
#include "../../Engine/engine_context.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

static unsigned int skyVAO = 0;
static unsigned int skyVBO = 0;

static void initSkybox()
{
    float vertices[] = {
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

    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);

    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void skybox_pass::render(EngineContext& ctx, const glm::mat4& view, const glm::mat4& projection)
{
    // Aria shader'ın ham OpenGL ID'sini alıyoruz
    unsigned int skyboxGLShaderID = ctx.aria_system->get_shader_id(ctx.skyboxShaderHandle);

    // 🛡️ EMNİYET KONTROLÜ: Shader yüklenemediyse boşuna çizim yapıp OpenGL'i ağlatma
    if (skyboxGLShaderID == 0)
    {
        // Gerekirse buraya bir log atabilirsin: "[Skybox] Shader handle invalid!"
        return;
    }

    if (skyVAO == 0)
        initSkybox();

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glUseProgram(skyboxGLShaderID); // Kesinlikle 0'dan büyük geçerli bir ID aktif oluyor

    glm::mat4 skyView = glm::mat4(glm::mat3(view));

    // Uniform lokasyonlarını güvenli bir şekilde alıp atıyoruz
    int skyboxLoc = glGetUniformLocation(skyboxGLShaderID, "skybox");
    if (skyboxLoc != -1) glUniform1i(skyboxLoc, 0);

    int viewLoc = glGetUniformLocation(skyboxGLShaderID, "view");
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(skyView));

    int projLoc = glGetUniformLocation(skyboxGLShaderID, "projection");
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(skyVAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.skyboxTexture);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    // Temizlik ve State Sıfırlama
    glBindVertexArray(0);
    glUseProgram(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // 🚨 NOT: glGetError() döngüsünü geçici olarak buraya çektik 
    // ki fonksiyonun kendi içindeki temiz çağrılardan emin olalım.
    /*GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        std::cout << "[SKYBOX ERR INNER] " << err << std::endl;*/

}