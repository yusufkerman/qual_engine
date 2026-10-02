#pragma once

/**
 * @file grid_renderer.h
 * @brief Sahne zemin ızgarası (Grid) için ekran/zemin quad geometrisini yöneten ve çizen sınıf.
 */

#include <glad/glad.h>

 /**
  * @class GridRenderer
  * @brief Sonsuz zemin ızgara shader'ı için gerekli olan 2B quad geometri tamponlarını yönetir.
  */
class GridRenderer
{
public:
    GridRenderer() = default;
    ~GridRenderer() = default;

    /**
     * @brief Quad tepe noktası tamponlarını (VAO ve VBO) oluşturur ve yapılandırır.
     */
    void init();

    /**
     * @brief Izgara quad geometrisini çizer.
     * @note Shader programı ve kamera uniform matrisleri bu çağrıdan önce bağlanmış olmalıdır.
     */
    void draw();

    /**
     * @brief Ayrılmış OpenGL tamponlarını serbest bırakır.
     */
    void shutdown();

private:
    unsigned int m_vao = 0; ///< Izgara quad tepe noktası dizi nesnesi (VAO).
    unsigned int m_vbo = 0; ///< Izgara quad tepe noktası tampon nesnesi (VBO).
};