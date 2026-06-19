#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::string loadFile(const char* path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        std::cout << "FILE NOT FOUND: " << path << std::endl;
        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::string vCode = loadFile(vertexPath);
    std::string fCode = loadFile(fragmentPath);

    const char* vShaderCode = vCode.c_str();
    const char* fShaderCode = fCode.c_str();

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vShaderCode, NULL);
    glCompileShader(vs);

    int success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cout << "VERTEX SHADER ERROR:\n" << infoLog << std::endl;
    }

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fShaderCode, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cout << "FRAGMENT SHADER ERROR:\n" << infoLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vs);
    glAttachShader(ID, fs);
    glLinkProgram(ID);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Shader::use()
{
    glUseProgram(ID);
}

void Shader::setFloat(const std::string& name, float value)
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}
void Shader::setVec2(const std::string& name, const glm::vec2& value)
{
    glUniform2f(glGetUniformLocation(ID, name.c_str()), value.x, value.y);
}