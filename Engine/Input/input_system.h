#pragma once

/**
 * @file input_system.h
 * @brief GLFW donanım olaylarını dinleyip girdi durumunu (InputState) güncelleyen sistem.
 */

struct EngineContext;
struct EditorContext;

/**
 * @class InputSystem
 * @brief Her karede donanım girdilerini sorgulayan ve eylemleri çözümleyen sınıf.
 */
class InputSystem
{
public:
    InputSystem() = default;
    ~InputSystem() = default;

    /**
     * @brief Donanım girdilerini okur, imleç kilit durumunu günceller ve eylemleri tetikler.
     * @param ctx Motorun çekirdek girdi ve pencere referansları.
     * @param editorCtx Editör bağlamı.
     */
    void update(EngineContext& ctx, EditorContext& editorCtx);

private:
    bool m_lastCursorLockState = false;
};