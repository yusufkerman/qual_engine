#pragma once

/**
 * @file opengl_debug.h
 * @brief OpenGL 4.5+ sürücü seviyesi hata ayıklama ve tanılama kancası (glDebugMessageCallback).
 * @details OpenGL API çağrılarında sürücüden dönen hataları, tanımsız davranışları,
 * bellek ve performans uyarılarını yakalar ve merkezi QualEngine Logger sistemine (Renderer kategorisi) aktarır.
 */

#include <glad/glad.h>

namespace qual::renderer {

/**
 * @brief OpenGL bağlamının debug yeteneklerini kontrol eder ve glDebugMessageCallback'i kaydeder.
 * @note Bu fonksiyon, gladLoadGLLoader çağrısından hemen sonra çağrılmalıdır.
 */
void setup_opengl_debug_output();

/**
 * @brief Sürücü tarafından OpenGL hatası veya uyarısı üretildiğinde tetiklenen geri çağırma (callback).
 */
void APIENTRY opengl_debug_message_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam);

} // namespace qual::renderer

