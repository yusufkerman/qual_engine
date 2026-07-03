#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class shader {
public:
    unsigned int ID;

    shader(unsigned int programID) : ID(programID) {}
    shader(const char* vertexPath, const char* fragmentPath);
    void use();
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const glm::vec2& value);
    void setMat4(const std::string& name, const glm::mat4& mat);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setInt(const std::string& name, int value);
};