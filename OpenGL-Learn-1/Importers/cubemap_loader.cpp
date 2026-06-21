#include "cubemap_loader.h"
#include <glad/glad.h>
#include "../Externals/stb/stb_image.h"
#include <iostream>

static unsigned int create_cubemap()
{
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texID;
}
unsigned int cubemap_loader::load_faces(const std::vector<std::string>& faces)
{
    unsigned int textureID = create_cubemap();

    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);

        if (!data)
        {
            std::cout
                << "[CUBEMAP ERROR] File not found or failed to load: "
                << faces[i] << std::endl;

            continue;
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

        std::cout
            << "[CUBEMAP] Loaded face " << i
            << " -> " << faces[i]
            << " (" << width << "x" << height << ")"
            << std::endl;
    }

    return textureID;
}
//4*3 loader
unsigned int cubemap_loader::load_atlas(const std::string& path)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data)
    {
        std::cout << "[CUBEMAP ATLAS ERROR] File not found or failed: " << path << std::endl;
        return 0;
    }

    int faceW = width / 4;
    int faceH = height / 3;

    if (faceW <= 0 || faceH <= 0)
    {
        std::cout << "[CUBEMAP ATLAS ERROR] Invalid atlas size!" << std::endl;
        stbi_image_free(data);
        return 0;
    }

    unsigned int texID = create_cubemap();
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    auto getPixel = [&](int x, int y, int c) {
        return data[(y * width + x) * channels + c];
        };

    auto uploadFace = [&](int fx, int fy, GLenum target, const std::string& name)
        {
            std::vector<unsigned char> faceData(faceW * faceH * channels);

            for (int y = 0; y < faceH; y++)
                for (int x = 0; x < faceW; x++)
                    for (int c = 0; c < channels; c++)
                        faceData[(y * faceW + x) * channels + c] =
                        getPixel(fx * faceW + x, fy * faceH + y, c);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(target, 0, format, faceW, faceH, 0, format, GL_UNSIGNED_BYTE, faceData.data());

            std::cout << "[CUBEMAP ATLAS] Face uploaded: " << name << std::endl;
        };

    // Haç düzeni: sütun(fx) 0..3, satır(fy) 0..2
    uploadFace(1, 0, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, "+Y"); // üst orta
    uploadFace(0, 1, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, "-X"); // orta sol
    uploadFace(1, 1, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, "+Z"); // orta orta (ön)
    uploadFace(2, 1, GL_TEXTURE_CUBE_MAP_POSITIVE_X, "+X"); // orta sağ
    uploadFace(3, 1, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, "-Z"); // orta en sağ
    uploadFace(1, 2, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, "-Y"); // alt orta

    stbi_image_free(data);
    return texID;
}