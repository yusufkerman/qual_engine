/**
 * @file shader.cpp
 * @brief [LEGACY] Gölgelendirici dosyalarının diskten okunması, derlenmesi ve bağlanması.
 */

#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Core/Logger/logger.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // Bu dosyanın kendi içindeki deprecated uyarılarını bastır
#endif

namespace {
    /**
     * @brief Belirtilen dosyayı okuyup string olarak döner.
     */
    std::string loadFile(const char* path)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            QLOG_ERROR(renderer, "[Shader] Dosya bulunamadi: {}", path);
            return "";
        }

        std::stringstream ss;
        ss << file.rdbuf();

        return ss.str();
    }
}

shader::shader(const char* vertexPath, const char* fragmentPath)
{
    const std::string vCode = loadFile(vertexPath);
    const std::string fCode = loadFile(fragmentPath);

    const char* vShaderCode = vCode.c_str();
    const char* fShaderCode = fCode.c_str();

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vShaderCode, NULL);
    glCompileShader(vs);

    int success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        QLOG_ERROR(renderer, "[Shader] Vertex shader derleme hatasi: {}", infoLog);
    }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fShaderCode, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        QLOG_ERROR(renderer, "[Shader] Fragment shader derleme hatasi: {}", infoLog);
    }

    ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        QLOG_ERROR(renderer, "[Shader] Shader baglama (link) hatasi: {}", infoLog);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void shader::use()
{
    glUseProgram(ID);
}

void shader::setFloat(const std::string& name, float value)
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void shader::setMat4(const std::string& name, const glm::mat4& mat)
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void shader::setVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void shader::setVec2(const std::string& name, const glm::vec2& value)
{
    glUniform2f(glGetUniformLocation(ID, name.c_str()), value.x, value.y);
}

void shader::setInt(const std::string& name, int value)
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif