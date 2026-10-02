#pragma once

/**
 * @file game_pass.h
 * @brief Oyun dünyası render geçişini (Game Viewport veya Standalone pencere) tanımlar.
 */

struct EngineContext;

namespace game_pass
{
    /**
     * @brief Sahnedeki aktif oyun kamerası üzerinden oyun dünyasını çizer.
     *
     * @details Editör yapılarında hedef FBO'ya (gameTarget), bağımsız sürümlerde
     * ise doğrudan varsayılan ekran tamponuna (default framebuffer) çizim yapar.
     *
     * @param ctx Motorun ana sistem, render ve sahne oturumu referansları.
     */
    void render(EngineContext& ctx);
}