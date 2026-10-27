#include <iostream>
#include <cassert>
#include <vector>
#include <string_view>

#include "../Engine/Core/Logger/logger.h"
#include "../Engine/Renderer/Debug/opengl_debug.h"

int main() {
    std::cout << "====================================================\n";
    std::cout << "[TEST] QualEngine Faz 3 OpenGL 4.5 Debug Hook Testi\n";
    std::cout << "====================================================\n\n";

    auto& logger = qual::log::engine_logger::instance();
    logger.clear();

    // 1. Simüle Edilmiş OpenGL Yüksek Öncelikli (HIGH) Hata Mesajı
    std::cout << "[Test 1] GL_DEBUG_SEVERITY_HIGH hata testi...\n";
    const char* high_msg = "GL_INVALID_OPERATION in glDrawElements: Index buffer not bound.";
    qual::renderer::opengl_debug_message_callback(
        GL_DEBUG_SOURCE_API,
        GL_DEBUG_TYPE_ERROR,
        1282, // GL_INVALID_OPERATION
        GL_DEBUG_SEVERITY_HIGH,
        static_cast<GLsizei>(std::strlen(high_msg)),
        high_msg,
        nullptr
    );

    std::vector<qual::log::log_entry> snapshot;
    logger.get_snapshot(snapshot);

    assert(snapshot.size() == 1);
    assert(snapshot[0].level == qual::log::log_level::critical);
    assert(snapshot[0].category == qual::log::log_category::renderer);
    assert(std::string_view(snapshot[0].message).find("GL_INVALID_OPERATION") != std::string_view::npos);
    std::cout << " -> Test 1 PASSED: High Severity kritik seviyede Logger'a aktarildi.\n\n";

    // 2. Simüle Edilmiş OpenGL Orta Öncelikli (MEDIUM) Hata Mesajı
    std::cout << "[Test 2] GL_DEBUG_SEVERITY_MEDIUM hata testi...\n";
    const char* med_msg = "Framebuffer incomplete, missing attachment.";
    qual::renderer::opengl_debug_message_callback(
        GL_DEBUG_SOURCE_API,
        GL_DEBUG_TYPE_ERROR,
        1286, // GL_INVALID_FRAMEBUFFER_OPERATION
        GL_DEBUG_SEVERITY_MEDIUM,
        static_cast<GLsizei>(std::strlen(med_msg)),
        med_msg,
        nullptr
    );

    logger.get_snapshot(snapshot);
    assert(snapshot.size() == 2);
    assert(snapshot[1].level == qual::log::log_level::error);
    assert(snapshot[1].category == qual::log::log_category::renderer);
    std::cout << " -> Test 2 PASSED: Medium Severity ERROR seviyesinde Logger'a aktarildi.\n\n";

    // 3. Simüle Edilmiş OpenGL Düşük Öncelikli (LOW) Performans Uyarısı
    std::cout << "[Test 3] GL_DEBUG_SEVERITY_LOW performans uyarisi testi...\n";
    const char* low_msg = "Redundant state change: Texture unit 0 already bound to texture 5.";
    qual::renderer::opengl_debug_message_callback(
        GL_DEBUG_SOURCE_API,
        GL_DEBUG_TYPE_PERFORMANCE,
        2001,
        GL_DEBUG_SEVERITY_LOW,
        static_cast<GLsizei>(std::strlen(low_msg)),
        low_msg,
        nullptr
    );

    logger.get_snapshot(snapshot);
    assert(snapshot.size() == 3);
    assert(snapshot[2].level == qual::log::log_level::warn);
    assert(snapshot[2].category == qual::log::log_category::renderer);
    std::cout << " -> Test 3 PASSED: Low Severity WARN seviyesinde Logger'a aktarildi.\n\n";

    // 4. Filtrelenen Mesajlar (Notification ve Zararsız Sürücü ID'leri)
    std::cout << "[Test 4] Bildirim ve bilinen zararsiz surucu ID filtreleme testi...\n";
    
    // a) Notification yoksayılmalı
    qual::renderer::opengl_debug_message_callback(
        GL_DEBUG_SOURCE_API,
        GL_DEBUG_TYPE_OTHER,
        9999,
        GL_DEBUG_SEVERITY_NOTIFICATION,
        12,
        "Notification",
        nullptr
    );

    // b) ID 131169 (NVIDIA buffer info) yoksayılmalı
    qual::renderer::opengl_debug_message_callback(
        GL_DEBUG_SOURCE_API,
        GL_DEBUG_TYPE_OTHER,
        131169,
        GL_DEBUG_SEVERITY_LOW,
        15,
        "Buffer allocated",
        nullptr
    );

    // c) ID 131185 (NVIDIA video memory) yoksayılmalı
    qual::renderer::opengl_debug_message_callback(
        GL_DEBUG_SOURCE_API,
        GL_DEBUG_TYPE_OTHER,
        131185,
        GL_DEBUG_SEVERITY_LOW,
        15,
        "Using VIDEO mem",
        nullptr
    );

    logger.get_snapshot(snapshot);
    // Boyut hâlâ 3 olmalı! (Yeni hiçbir şey eklenmemeli)
    assert(snapshot.size() == 3);
    std::cout << " -> Test 4 PASSED: Bildirimler ve ID 131169/131185 basariyla filtrelendi, log gurultusu onlendi.\n\n";

    std::cout << "====================================================\n";
    std::cout << " [FAZ 3 HEPSI BASARILI] OPENGL 4.5 DEBUG HOOK %100 DOGRULANDI\n";
    std::cout << "====================================================\n";
    return 0;
}

