#pragma once

/**
 * @file shader.h
 * @brief [LEGACY] OpenGL gölgelendirici (shader) derleme ve uniform yönetim sarmalayıcısı.
 */

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

 /**
  * @class shader
  * @brief [LEGACY] Ham OpenGL shader derleme ve uniform atama işlemlerini yürüten sınıf.
  * @deprecated Aria Asset System'e geçildiği için bu sınıf eski sistemdir. Varlık yönetimi için aria::shader_asset kullanınız.
  */
class [[deprecated("Aria Asset System'e gecildigi icin aria::shader_asset kullaniniz.")]] shader
{
public:
    unsigned int ID;

    /**
     * @brief Mevcut bir OpenGL program ID'si ile shader nesnesi oluşturur.
     */
    shader(unsigned int programID) : ID(programID) {}

    /**
     * @brief Belirtilen dosya yollarından Vertex ve Fragment shader kodlarını okuyup derler.
     * @param vertexPath Vertex shader dosya yolu.
     * @param fragmentPath Fragment shader dosya yolu.
     */
    shader(const char* vertexPath, const char* fragmentPath);

    /**
     * @brief Bu shader programını aktif OpenGL durumu (State) olarak ayarlar (glUseProgram).
     */
    void use();

    // Uniform atama (Set) fonksiyonları
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const glm::vec2& value);
    void setMat4(const std::string& name, const glm::mat4& mat);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setInt(const std::string& name, int value);
};