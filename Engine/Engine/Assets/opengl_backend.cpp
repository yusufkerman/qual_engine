/**
 * @file opengl_backend.cpp
 * @brief OpenGL spesifik render arka ucu (backend) uygulaması.
 */

#include "opengl_backend.h"
#include <glad/glad.h>
#include <glm/common.hpp>
#include <iostream>
#include "../../Core/Logger/logger.h"

namespace aria {

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

        // Doku Filtreleme Ayarları
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Anizotropik Filtreleme
        float max_aniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, glm::min(8.0f, max_aniso));

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex_id;
    }

    std::vector<submesh_bridge> opengl_backend::create_model(const raw_model_data& raw_data) {
        std::vector<submesh_bridge> out_submeshes;
        out_submeshes.reserve(raw_data.sub_meshes.size());

        for (const auto& raw_mesh : raw_data.sub_meshes) {
            submesh_bridge submesh;

            glGenVertexArrays(1, &submesh.gpu_data.vao_or_buffer_id);
            glGenBuffers(1, &submesh.gpu_data.vbo);
            glGenBuffers(1, &submesh.gpu_data.ebo);

            glBindVertexArray(submesh.gpu_data.vao_or_buffer_id);

            glBindBuffer(GL_ARRAY_BUFFER, submesh.gpu_data.vbo);
            glBufferData(GL_ARRAY_BUFFER, raw_mesh.vertices.size() * sizeof(raw_vertex), raw_mesh.vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.gpu_data.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, raw_mesh.indices.size() * sizeof(uint32_t), raw_mesh.indices.data(), GL_STATIC_DRAW);

            // Pozisyon (location = 0)
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, position));

            // Normal (location = 1)
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, normal));

            // UV Koordinatları (location = 2)
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, tex_coord));

            // Tanjant (location = 3)
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(raw_vertex), (void*)offsetof(raw_vertex, tangent));

            submesh.bounds.local_center = raw_mesh.local_center;
            submesh.bounds.local_radius = raw_mesh.local_radius;

            glBindVertexArray(0);
            out_submeshes.push_back(submesh);
        }

        return out_submeshes;
    }

    std::vector<submesh_bridge> opengl_backend::create_skinned_model(const raw_skinned_model_data& raw_data) {
        std::vector<submesh_bridge> out_submeshes;
        out_submeshes.reserve(raw_data.sub_meshes.size());

        for (const auto& raw_mesh : raw_data.sub_meshes) {
            submesh_bridge submesh;

            glGenVertexArrays(1, &submesh.gpu_data.vao_or_buffer_id);
            glGenBuffers(1, &submesh.gpu_data.vbo);
            glGenBuffers(1, &submesh.gpu_data.ebo);

            glBindVertexArray(submesh.gpu_data.vao_or_buffer_id);

            glBindBuffer(GL_ARRAY_BUFFER, submesh.gpu_data.vbo);
            glBufferData(GL_ARRAY_BUFFER, raw_mesh.vertices.size() * sizeof(raw_skinned_vertex), raw_mesh.vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.gpu_data.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, raw_mesh.indices.size() * sizeof(uint32_t), raw_mesh.indices.data(), GL_STATIC_DRAW);

            // Pozisyon (location = 0)
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, position));

            // Normal (location = 1)
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, normal));

            // UV Koordinatları (location = 2)
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, tex_coord));

            // Tanjant (location = 3)
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, tangent));

            // Kemik ID'leri (location = 4 - KRİTİK: Tamsayı için glVertexAttribIPointer!)
            glEnableVertexAttribArray(4);
            glVertexAttribIPointer(4, 4, GL_INT, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, bone_ids));

            // Kemik Ağırlıkları (location = 5)
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(raw_skinned_vertex), (void*)offsetof(raw_skinned_vertex, bone_weights));

            submesh.bounds.local_center = raw_mesh.local_center;
            submesh.bounds.local_radius = raw_mesh.local_radius;

            glBindVertexArray(0);
            out_submeshes.push_back(submesh);
        }

        return out_submeshes;
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

    gpu_shader_id opengl_backend::create_shader(const raw_shader_data& raw_data) {
        if (raw_data.vertex_source.empty() || raw_data.fragment_source.empty()) {
            std::cout << "[ARIA OpenGL] Hata: Shader kaynak kodlari bos!\n";
            return 0;
        }

        const char* vShaderCode = raw_data.vertex_source.c_str();
        const char* fShaderCode = raw_data.fragment_source.c_str();

        int success;
        char infoLog[512];

        // Vertex Shader Derleme
        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vShaderCode, NULL);
        glCompileShader(vs);
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vs, 512, NULL, infoLog);
            QLOG_ERROR(renderer, "[ARIA OpenGL] Vertex Shader Derleme Hatasi: {}", infoLog);
            glDeleteShader(vs);
            return 0;
        }

        // Fragment Shader Derleme
        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fShaderCode, NULL);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fs, 512, NULL, infoLog);
            QLOG_ERROR(renderer, "[ARIA OpenGL] Fragment Shader Derleme Hatasi: {}", infoLog);
            glDeleteShader(vs);
            glDeleteShader(fs);
            return 0;
        }

        // Program Baglama (Linking)
        GLuint programID = glCreateProgram();
        glAttachShader(programID, vs);
        glAttachShader(programID, fs);
        glLinkProgram(programID);

        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            QLOG_ERROR(renderer, "[ARIA OpenGL] Program Baglama Hatasi: {}", infoLog);
            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(programID);
            return 0;
        }

        // UBO (Uniform Buffer Object) Entegrasyonu
        unsigned int blockIndex = glGetUniformBlockIndex(programID, "LightBuffer");
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(programID, blockIndex, 0);
        }
        else {
            QLOG_WARN(renderer, "[ARIA OpenGL] Uyari: LightBuffer uniform blogu shader ({}) icinde bulunamadi!", programID);
        }

        // İskelet / Deri Yüzme Kemik Matris Bloğu Entegrasyonu
        unsigned int boneBlockIndex = glGetUniformBlockIndex(programID, "BoneMatricesBlock");
        if (boneBlockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(programID, boneBlockIndex, 1);
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        return programID;
    }

    gpu_shader_id opengl_backend::create_fallback_shader() {
        // Hata durumunda sahneyi pembe (magenta) boyayan basit yedek gölgelendirici
        const char* vsSource = "#version 330 core\nlayout (location = 0) in vec3 aPos;\nuniform mat4 model;\nuniform mat4 view;\nuniform mat4 projection;\nvoid main() { gl_Position = projection * view * model * vec4(aPos, 1.0); }";
        const char* fsSource = "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0, 0.0, 1.0, 1.0); }";

        int success;
        char infoLog[512];

        unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vsSource, NULL);
        glCompileShader(vs);
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vs, 512, NULL, infoLog);
            QLOG_ERROR(renderer, "[ARIA OpenGL] Yedek (Fallback) Vertex Shader Hatasi: {}", infoLog);
        }

        unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fsSource, NULL);
        glCompileShader(fs);
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fs, 512, NULL, infoLog);
            QLOG_ERROR(renderer, "[ARIA OpenGL] Yedek (Fallback) Fragment Shader Hatasi: {}", infoLog);
        }

        GLuint programID = glCreateProgram();
        glAttachShader(programID, vs);
        glAttachShader(programID, fs);
        glLinkProgram(programID);

        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            QLOG_ERROR(renderer, "[ARIA OpenGL] Yedek (Fallback) Program Baglama Hatasi: {}", infoLog);
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        return programID;
    }

    void opengl_backend::destroy_shader(gpu_shader_id id) {
        if (id != 0) glDeleteProgram(id);
    }

    void opengl_backend::reset_state() {
        // OpenGL durum makinesini (state machine) varsayılana döndür
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);

        // Doku (Texture) yuvalarını sıfırla
        for (int i = 0; i < 16; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

} // namespace aria