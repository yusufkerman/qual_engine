/**
 * @file cubemap_loader.cpp
 * @brief Küp haritası yükleme ve doku oluşturma mantıksal uygulaması.
 */

#include "cubemap_loader.h"
#include <glad/glad.h>
#include "../Externals/stb/stb_image.h"
#include "../Core/VFS/vfs.h"
#include "../Resources/engine_resources.h"
#include "../Core/path_utils.h"
#include "../Project/project.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <algorithm>

namespace {

    /**
     * @brief Küp haritası için filtre ve kenar sarma (clamp-to-edge) ayarlarını yapılandırır.
     */
    unsigned int create_cubemap()
    {
        unsigned int texID = 0;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        return texID;
    }

} // anonymous namespace

unsigned int cubemap_loader::load_faces(const std::vector<std::string>& faces)
{
    if (faces.size() != 6) {
        std::cerr << "[CubemapLoader] Gecersiz yuz sayisi (6 yuz bekleniyordu): " << faces.size() << '\n';
        return 0;
    }

    unsigned int textureID = create_cubemap();

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < 6; ++i)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);

        if (!data)
        {
            std::cerr << "[CubemapLoader] Dosya yuklenemedi: " << faces[i] << '\n';
            glDeleteTextures(1, &textureID);
            return 0;
        }

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            format,
            width,
            height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            data
        );

        stbi_image_free(data);
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

unsigned int cubemap_loader::load_atlas(const std::string& inputPath)
{
    if (inputPath.empty()) return 0;

    std::string path = inputPath;
    std::replace(path.begin(), path.end(), '\\', '/');

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);

    unsigned char* data = nullptr;
    std::vector<uint8_t> vfsBuffer;

    // 1. Motor dahili kaynagi (@engine/...)
    if (path.rfind("@engine", 0) == 0) {
        std::string resolved = engine_resources::resolve_path(path);
        if (std::filesystem::exists(qual::path_from_utf8(resolved))) {
            data = stbi_load(resolved.c_str(), &width, &height, &channels, 0);
        }
    }

    // 2. VFS Paket arsivi (data.pak) kontrolu
    if (!data && qual::vfs::instance().is_mounted()) {
        if (qual::vfs::instance().has_file(path)) {
            vfsBuffer = qual::vfs::instance().read_bytes(path);
        } else {
            size_t aIdx = path.find("Assets/");
            if (aIdx != std::string::npos) {
                std::string relAssets = path.substr(aIdx);
                if (qual::vfs::instance().has_file(relAssets)) {
                    vfsBuffer = qual::vfs::instance().read_bytes(relAssets);
                }
            }
        }
        if (!vfsBuffer.empty()) {
            data = stbi_load_from_memory(vfsBuffer.data(), static_cast<int>(vfsBuffer.size()), &width, &height, &channels, 0);
        }
    }

    // 3. Fiziksel dosya yolu (oldugu gibi)
    if (!data && std::filesystem::exists(qual::path_from_utf8(path))) {
        data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }

    // 3b. Aktif proje kök dizini (project root) kontrolü
    if (!data && project::project_manager::instance().has_active_project()) {
        const std::filesystem::path projRoot = qual::path_from_utf8(project::project_manager::instance().get_project_root());
        std::filesystem::path fullCandidate = projRoot / qual::path_from_utf8(path);
        if (std::filesystem::exists(fullCandidate)) {
            data = stbi_load(qual::path_to_utf8(fullCandidate).c_str(), &width, &height, &channels, 0);
        } else {
            size_t aIdx = path.find("Assets/");
            if (aIdx != std::string::npos) {
                fullCandidate = projRoot / qual::path_from_utf8(path.substr(aIdx));
                if (std::filesystem::exists(fullCandidate)) {
                    data = stbi_load(qual::path_to_utf8(fullCandidate).c_str(), &width, &height, &channels, 0);
                }
            }
        }
    }

    // 3c. Mevcut çalışma dizinine göre göreceli kontrol
    if (!data) {
        std::filesystem::path curCandidate = std::filesystem::current_path() / qual::path_from_utf8(path);
        if (std::filesystem::exists(curCandidate)) {
            data = stbi_load(qual::path_to_utf8(curCandidate).c_str(), &width, &height, &channels, 0);
        }
    }

    // 4. Assets/ ile baslayan goreceli yol kontrolu
    if (!data) {
        size_t aIdx = path.find("Assets/");
        if (aIdx != std::string::npos) {
            std::string rel = path.substr(aIdx);
            if (std::filesystem::exists(qual::path_from_utf8(rel))) {
                data = stbi_load(rel.c_str(), &width, &height, &channels, 0);
            }
        }
    }

    if (!data)
    {
        std::cerr << "[CubemapLoader] Atlas dosyasi bulunamadi: " << inputPath << '\n';
        return 0;
    }

    const int faceW = width / 4;
    const int faceH = height / 3;

    if (faceW <= 0 || faceH <= 0)
    {
        std::cerr << "[CubemapLoader] Gecersiz atlas cozunurlugu: " << width << "x" << height << '\n';
        stbi_image_free(data);
        return 0;
    }

    unsigned int texID = create_cubemap();
    const GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    auto uploadFace = [&](int fx, int fy, GLenum target)
        {
            std::vector<unsigned char> faceData(faceW * faceH * channels);

            // Satır satır bellek kopyalama ile alt dikdörtgeni ayıkla
            for (int row = 0; row < faceH; ++row)
            {
                const unsigned char* src = data + (((fy * faceH + row) * width + (fx * faceW)) * channels);
                unsigned char* dst = faceData.data() + (row * faceW * channels);
                std::memcpy(dst, src, faceW * channels);
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(target, 0, format, faceW, faceH, 0, format, GL_UNSIGNED_BYTE, faceData.data());
        };

    // Standart 4x3 Haç (Cross) yerleşimi:
    //      [+Y]
    // [-X] [+Z] [+X] [-Z]
    //      [-Y]
    uploadFace(1, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_Y); // Üst Orta (+Y)
    uploadFace(0, 1, GL_TEXTURE_CUBE_MAP_NEGATIVE_X); // Orta Sol (-X)
    uploadFace(1, 1, GL_TEXTURE_CUBE_MAP_POSITIVE_Z); // Orta (+Z / Ön)
    uploadFace(2, 1, GL_TEXTURE_CUBE_MAP_POSITIVE_X); // Orta Sağ (+X)
    uploadFace(3, 1, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z); // En Sağ (-Z / Arka)
    uploadFace(1, 2, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y); // Alt Orta (-Y)

    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    stbi_image_free(data);
    return texID;
}