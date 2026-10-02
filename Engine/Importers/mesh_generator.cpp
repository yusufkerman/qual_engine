/**
 * @file mesh_generator.cpp
 * @brief Prosedürel ağ üretim algoritmalarının matematiksel uygulaması.
 */

#include "mesh_generator.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace mesh_generator
{
    generated_mesh_data generate_cube()
    {
        generated_mesh_data data;

        // 6 yüzey * 4 tepe noktası = 24 Vertex (Her yüzeyin bağımsız normal vektörü bulunur)
        data.vertices = {
            // Ön Yüz (Vertices: 0-3, Normal: 0, 0, 1)
            { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
            { { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
            { { 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
            { {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },

            // Arka Yüz (Vertices: 4-7, Normal: 0, 0, -1)
            { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} },
            { {-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} },
            { { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} },
            { { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} },

            // Sol Yüz (Vertices: 8-11, Normal: -1, 0, 0)
            { {-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
            { {-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
            { {-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
            { {-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },

            // Sağ Yüz (Vertices: 12-15, Normal: 1, 0, 0)
            { { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
            { { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} },
            { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} },
            { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },

            // Üst Yüz (Vertices: 16-19, Normal: 0, 1, 0)
            { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} },
            { {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} },
            { { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
            { { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} },

            // Alt Yüz (Vertices: 20-23, Normal: 0, -1, 0)
            { {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
            { { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },
            { { 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} },
            { {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} }
        };

        // 6 yüzey * 2 üçgen * 3 indeks = 36 İndis (Saat yönünün tersi - CCW)
        data.indices = {
            0,  1,  2,  2,  3,  0,  // Ön
            4,  5,  6,  6,  7,  4,  // Arka
            8,  9,  10, 10, 11, 8,  // Sol
            12, 13, 14, 14, 15, 12, // Sağ
            16, 17, 18, 18, 19, 16, // Üst
            20, 21, 22, 22, 23, 20  // Alt
        };

        return data;
    }

    generated_mesh_data generate_sphere(float radius, unsigned int sector_count, unsigned int stack_count)
    {
        generated_mesh_data data;

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float xy = 0.0f;

        const float length_inv = 1.0f / radius;
        const float sector_step = 2.0f * glm::pi<float>() / static_cast<float>(sector_count);
        const float stack_step = glm::pi<float>() / static_cast<float>(stack_count);

        for (unsigned int i = 0; i <= stack_count; ++i)
        {
            const float stack_angle = (glm::pi<float>() * 0.5f) - (static_cast<float>(i) * stack_step);
            xy = radius * std::cos(stack_angle);
            z = radius * std::sin(stack_angle);

            for (unsigned int j = 0; j <= sector_count; ++j)
            {
                const float sector_angle = static_cast<float>(j) * sector_step;

                x = xy * std::cos(sector_angle);
                y = xy * std::sin(sector_angle);

                const float nx = x * length_inv;
                const float ny = y * length_inv;
                const float nz = z * length_inv;

                const float s = static_cast<float>(j) / static_cast<float>(sector_count);
                const float t = static_cast<float>(i) / static_cast<float>(stack_count);

                data.vertices.push_back({ {x, y, z}, {nx, ny, nz}, {s, t} });
            }
        }

        // İndis üretimi (Üçgen yüzey tanımlamaları)
        for (unsigned int i = 0; i < stack_count; ++i)
        {
            unsigned int k1 = i * (sector_count + 1);
            unsigned int k2 = k1 + sector_count + 1;

            for (unsigned int j = 0; j < sector_count; ++j, ++k1, ++k2)
            {
                if (i != 0)
                {
                    data.indices.push_back(k1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k1 + 1);
                }

                if (i != (stack_count - 1))
                {
                    data.indices.push_back(k1 + 1);
                    data.indices.push_back(k2);
                    data.indices.push_back(k2 + 1);
                }
            }
        }

        return data;
    }
}