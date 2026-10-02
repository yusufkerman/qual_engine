#pragma once

/**
 * @file texture_manager.h
 * @brief [LEGACY / DEPRECATED] Eski bellek içi doku kayıt defteri ve fallback doku yöneticisi.
 * @deprecated Doku yönetimi ve GPU yaşam döngüsü tamamen Aria Asset System mimarisine devredilmiştir.
 */

#include <string>
#include <unordered_map>

 /**
  * @class texture_manager
  * @brief Eski doku yöneticisi sınıfı.
  * @deprecated Aria Asset System handle/GUID mimarisine geçildiği için kullanımdan kaldırılmıştır.
  */
class [[deprecated("Doku islemleri icin Aria Asset System kullaniniz.")]] texture_manager
{
public:
    texture_manager();
    ~texture_manager();

    /**
     * @brief 1x1 beyaz yedek (fallback) dokuyu VRAM üzerinde oluşturur.
     */
    void init();

    /**
     * @brief Belirtilen kimliğe sahip bir doku kaydı olup olmadığını sorgular.
     */
    [[nodiscard]] bool has(unsigned int id) const;

    /**
     * @brief Dosya yolundan dokuyu yükler ve yerel kimlik atar.
     */
    unsigned int load(const std::string& path);

    /**
     * @brief Kimliği verilen dokunun OpenGL tanıtıcısını döner. Bulunamazsa fallback döner.
     */
    [[nodiscard]] unsigned int get(unsigned int id);

    /**
     * @brief Kimliği verilen dokunun dosya yolunu döner.
     */
    [[nodiscard]] const std::string& get_path(unsigned int id) const;

    /**
     * @brief Kayıtlı tüm doku yollarını döner.
     */
    [[nodiscard]] const std::unordered_map<unsigned int, std::string>& get_all_paths() const noexcept
    {
        return texturePaths;
    }

    /**
     * @brief VRAM üzerindeki tüm dokuları serbest bırakır ve kayıtları sıfırlar.
     */
    void clear();

private:
    std::unordered_map<unsigned int, unsigned int> textures;      ///< Yerel ID -> OpenGL Texture ID
    std::unordered_map<unsigned int, std::string> texturePaths;   ///< Yerel ID -> Dosya Yolu
    unsigned int nextID = 0;

    unsigned int m_FallbackTextureGLID = 0;                       ///< 1x1 beyaz varsayılan doku kimliği.
};