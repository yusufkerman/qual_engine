// Dosya Yolu: Importers/mesh_generator.cpp
#include "mesh_generator.h"
#include <glm/gtc/constants.hpp>

namespace mesh_generator
{
    generated_mesh_data generate_cube()
    {
        generated_mesh_data data;

        // Tam olarak 24 Vertex (6 yüzey * 4 vertex)
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

        // Tam olarak 36 İndis (6 yüzey * 2 üçgen * 3 indeks)
        // Saat yönünün tersi (Counter-Clockwise - CCW) kuralına göre sıralandı
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

        float x, y, z, xy;                              // vertex konumları
        float nx, ny, nz, length_inv = 1.0f / radius;    // vertex normalleri
        float s, t;                                     // vertex UV'leri

        float sector_step = 2 * glm::pi<float>() / sector_count;
        float stack_step = glm::pi<float>() / stack_count;
        float sector_angle, stack_angle;

        for (unsigned int i = 0; i <= stack_count; ++i)
        {
            stack_angle = glm::pi<float>() / 2 - i * stack_step; // pi/2 den -pi/2 ye
            xy = radius * cosf(stack_angle);
            z = radius * sinf(stack_angle);

            for (unsigned int j = 0; j <= sector_count; ++j)
            {
                sector_angle = j * sector_step;

                // Pozisyon
                x = xy * cosf(sector_angle);
                y = xy * sinf(sector_angle);

                // Normal
                nx = x * length_inv;
                ny = y * length_inv;
                nz = z * length_inv;

                // UV koordinatları
                s = (float)j / sector_count;
                t = (float)i / stack_count;

                data.vertices.push_back({ {x, y, z}, {nx, ny, nz}, {s, t} });
            }
        }

        // Index üretimi (Üçgen tanımları)
        unsigned int k1, k2;
        for (unsigned int i = 0; i < stack_count; ++i)
        {
            k1 = i * (sector_count + 1);     // mevcut stack başlangıcı
            k2 = k1 + sector_count + 1;     // sonraki stack başlangıcı

            for (unsigned int j = 0; j < sector_count; ++j, ++k1, ++k2)
            {
                // Kutup noktaları harici her quadrant için 2 üçgen oluşturuyoruz
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