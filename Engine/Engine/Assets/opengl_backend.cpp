#include "opengl_backend.h"
#include <glad/glad.h>
#include <glm/common.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

namespace aria {

    // Yardımcı dosya okuyucu (Eski kodundan aktarıldı)
    static std::string loadShaderFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cout << "[Aria OpenGL] SHADER FILE NOT FOUND: " << path << std::endl;
            return "";
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    void opengl_backend::init() {}
    void opengl_backend::shutdown() {}

    gpu_texture_id opengl_backend::create_texture(const raw_texture_data& raw_data) {
        GLenum format = GL_RGB;
        if (raw_data.channels == 1)      format = GL_RED;
        else if (raw_data.channels == 3) format = GL_RGB;
        else if (raw_data.channels == 4) format = GL_RGBA;

        GLuint tex_id;
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);

        glTexImage2D(GL_TEXTURE_2D, 0, format, raw_data.width, raw_data.height, 0, format, GL_UNSIGNED_BYTE, raw_data.pixels.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        float max_aniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso); // İsim düzeltmesi
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, glm::min(8.0f, max_aniso));

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex_id;
    }

    std::vector<gpu_mesh_id> opengl_backend::create_model(const raw_model_data& raw_data) {
        std::vector<gpu_mesh_id> out_ids;
        out_ids.reserve(raw_data.sub_meshes.size());

        for (const auto& raw_mesh : raw_data.sub_meshes) {
            gpu_mesh_id mesh_id;

            glGenVertexArrays(1, &mesh_id.vao_or_buffer_id);
            glGenBuffers(1, &mesh_id.vbo);
            glGenBuffers(1, &mesh_id.ebo);

            glBindVertexArray(mesh_id.vao_or_buffer_id);

            glBindBuffer(GL_ARRAY_BUFFER, mesh_id.vbo);
            glBufferData(GL_ARRAY_BUFFER, raw_mesh.vertices.size() * sizeof(raw_vertex), raw_mesh.vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_id.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, raw_mesh.indices.size() * sizeof(uint32_t), raw_mesh.indices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, position));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, normal));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, tex_coord));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, tangent));

            GLint enabled;

            for (int i = 0;i < 4;i++)
            {
                glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);

                std::cout
                    << "Attrib "
                    << i
                    << " enabled="
                    << enabled
                    << std::endl;
            }

            glBindVertexArray(0);
            out_ids.push_back(mesh_id);
        }
        return out_ids;
    }

    gpu_texture_id opengl_backend::create_fallback_texture() {
        GLuint tex_id;
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);

        unsigned char white_pixel[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        return tex_id;
    }

    void opengl_backend::destroy_texture(gpu_texture_id id) {
        if (id != 0) glDeleteTextures(1, &id);
    }

    void opengl_backend::destroy_model(const std::vector<gpu_mesh_id>& mesh_ids) {
        for (const auto& id : mesh_ids) {
            glDeleteVertexArrays(1, &id.vao_or_buffer_id);
            glDeleteBuffers(1, &id.vbo);
            glDeleteBuffers(1, &id.ebo);
        }
    }

    // 🔥 ESKİ SHADER SINIFI MANTIĞININ ARIA BACKEND'E ENJEKTE EDİLMİŞ HALİ
    gpu_shader_id opengl_backend::create_shader(const raw_shader_data& raw_data) {
        std::cout << "[ARIA OpenGL] create_shader called - vertex size: " << raw_data.vertex_source.size()
            << " fragment size: " << raw_data.fragment_source.size() << "\n";

        if (raw_data.vertex_source.empty() || raw_data.fragment_source.empty()) {
            std::cout << "[ARIA OpenGL] ERROR: Shader source is empty!\n";
            return 0;
        }

        const char* vShaderCode = raw_data.vertex_source.c_str();
        const char* fShaderCode = raw_data.fragment_source.c_str();

        int success;
        char infoLog[512];

        // Vertex Shader
        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vShaderCode, NULL);
        glCompileShader(vs);
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vs, 512, NULL, infoLog);
            std::cout << "[ARIA OpenGL] VERTEX SHADER COMPILE ERROR:\n" << infoLog << "\n";
            glDeleteShader(vs);
            return 0;
        }

        // Fragment Shader
        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fShaderCode, NULL);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fs, 512, NULL, infoLog);
            std::cout << "[ARIA OpenGL] FRAGMENT SHADER COMPILE ERROR:\n" << infoLog << "\n";
            glDeleteShader(vs);
            glDeleteShader(fs);
            return 0;
        }

        // Program Linking
        GLuint programID = glCreateProgram();
        glAttachShader(programID, vs);
        glAttachShader(programID, fs);
        glLinkProgram(programID);

        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            std::cout << "[ARIA OpenGL] PROGRAM LINK ERROR:\n" << infoLog << "\n";
            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(programID);
            return 0;
        }

        // UBO ENTEGRASYONU
        // Shader içinde "layout (std140) uniform LightBuffer" tanımlıysa 0. yuvaya bağla
        unsigned int blockIndex = glGetUniformBlockIndex(programID, "LightBuffer");
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(programID, blockIndex, 0);
            std::cout << "[ARIA OpenGL] LightBuffer bound to binding point 0 for program " << programID << "\n";
            
            // === YENİ EKLENEN DEBUG SATIRLARI ===
            GLint blockSize = 0;
            glGetActiveUniformBlockiv(programID, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
            std::cout << "[UBO CHECK] Program " << programID << " LightBuffer expects " << blockSize << " bytes\n";
            // =====================================
        
        }
        else {
            std::cout << "[ARIA OpenGL] WARNING: LightBuffer uniform block NOT found in shader " << programID << "!\n";
        }


        glDeleteShader(vs);
        glDeleteShader(fs);

        std::cout << "[ARIA OpenGL] Shader successfully created with GPU ID: " << programID << "\n";
        return programID;
    }

    // 🔥 Hata durumlarında motorun patlamaması için basit magenta/pembe basan emniyet shader'ı
    gpu_shader_id opengl_backend::create_fallback_shader() {
        const char* vsSource = "#version 330 core\nlayout (location = 0) in vec3 aPos;\nuniform mat4 model;\nuniform mat4 view;\nuniform mat4 projection;\nvoid main() { gl_Position = projection * view * model * vec4(aPos, 1.0); }";
        const char* fsSource = "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0, 0.0, 1.0, 1.0); }"; // Fallback magenta color for error detection

        int success;
        char infoLog[512];

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vsSource, NULL);
        glCompileShader(vs);
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vs, 512, NULL, infoLog);
            std::cout << "[ARIA OpenGL] FALLBACK VERTEX SHADER ERROR:\n" << infoLog << "\n";
        }

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fsSource, NULL);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fs, 512, NULL, infoLog);
            std::cout << "[ARIA OpenGL] FALLBACK FRAGMENT SHADER ERROR:\n" << infoLog << "\n";
        }

        GLuint programID = glCreateProgram();
        glAttachShader(programID, vs);
        glAttachShader(programID, fs);
        glLinkProgram(programID);

        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            std::cout << "[ARIA OpenGL] FALLBACK PROGRAM LINK ERROR:\n" << infoLog << "\n";
        }

        std::cout << "[ARIA OpenGL] Fallback shader created with ID: " << programID << "\n";

        glDeleteShader(vs);
        glDeleteShader(fs);

        return programID;
    }

    void opengl_backend::destroy_shader(gpu_shader_id id) {
        if (id != 0) glDeleteProgram(id);
    }


    void aria::opengl_backend::reset_state() {
        // OpenGL state makinesini varsayılana döndür
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);

        // Gerekirse texture slotlarını sıfırla
        for (int i = 0; i < 16; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

} // namespace aria