/**
 * @file opengl_debug.cpp
 * @brief OpenGL 4.5+ glDebugMessageCallback uygulaması ve sürücü mesaj filtreleme mantığı.
 */

#include "opengl_debug.h"
#include "../../Core/Logger/logger.h"

namespace qual::renderer {

namespace {

[[nodiscard]] const char* debug_source_to_string(GLenum source) noexcept {
    switch (source) {
        case GL_DEBUG_SOURCE_API:             return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY:     return "Third Party";
        case GL_DEBUG_SOURCE_APPLICATION:     return "Application";
        case GL_DEBUG_SOURCE_OTHER:           return "Other";
        default:                              return "Unknown";
    }
}

[[nodiscard]] const char* debug_type_to_string(GLenum type) noexcept {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "Undefined Behavior";
        case GL_DEBUG_TYPE_PORTABILITY:         return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE:         return "Performance";
        case GL_DEBUG_TYPE_MARKER:              return "Marker";
        case GL_DEBUG_TYPE_PUSH_GROUP:          return "Push Group";
        case GL_DEBUG_TYPE_POP_GROUP:           return "Pop Group";
        case GL_DEBUG_TYPE_OTHER:               return "Other";
        default:                                return "Unknown";
    }
}

} // namespace

void APIENTRY opengl_debug_message_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei /*length*/,
    const GLchar* message,
    const void* /*userParam*/)
{
    // 1. Bilgilendirme / Bildirim (Notification) mesajlarını yoksay
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }

    // 2. Zararsız NVIDIA/AMD sürücü bellek / shader optimizasyon bildirimlerini filtrele
    // 131169: "Framebuffer detailed info: The driver allocated storage for renderbuffer..."
    // 131185: "Buffer detailed info: Buffer object will use VIDEO memory..."
    // 131218: "Program/shader state performance warning: Fragment shader in program ... is being recompiled..."
    // 131204: "Texture state usage warning..."
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    const char* src_str = debug_source_to_string(source);
    const char* type_str = debug_type_to_string(type);

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            QLOG_CRITICAL(renderer, "[GL High Error] [{}:{}] ID: {} - {}", src_str, type_str, id, message);
            break;

        case GL_DEBUG_SEVERITY_MEDIUM:
            QLOG_ERROR(renderer, "[GL Error] [{}:{}] ID: {} - {}", src_str, type_str, id, message);
            break;

        case GL_DEBUG_SEVERITY_LOW:
            QLOG_WARN(renderer, "[GL Warning] [{}:{}] ID: {} - {}", src_str, type_str, id, message);
            break;

        default:
            QLOG_DEBUG(renderer, "[GL Debug] [{}:{}] ID: {} - {}", src_str, type_str, id, message);
            break;
    }
}

void setup_opengl_debug_output()
{
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);

    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(opengl_debug_message_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

        // Gürültülü bildirimleri (GL_DEBUG_SEVERITY_NOTIFICATION) filtrele
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

        QLOG_INFO(renderer, "OpenGL 4.5 Core baglami ve Debug Kancasi (glDebugMessageCallback) aktiflestirildi.");
    } else {
        QLOG_INFO(renderer, "OpenGL 4.5 Core baglami basariyla yuklendi (Debug Context pasif).");
    }
}

} // namespace qual::renderer

