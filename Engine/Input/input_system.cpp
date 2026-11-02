/**
 * @file input_system.cpp
 * @brief Girdi okuma ve eylem eşleme döngüsünün uygulaması.
 */

#include "input_system.h"
#include "../Editor/editor_camera.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"
#include "input_state.h"
#include "input_map.h"
#include <GLFW/glfw3.h>

void InputSystem::update(EngineContext& ctx, EditorContext& editorCtx)
{
    auto& game = ctx.vestaSys.viewport_state().game();

    ctx.inputState.gameViewportSize = game.size;
    ctx.inputState.gameViewportPos = game.position;

    GLFWwindow* window = ctx.window;
    InputState& state = ctx.inputState;
    const InputMap& map = ctx.inputMap;

    // Editör modunda oyun çalışmıyorsa veya duraklatılmışsa imleç asla kilitli kalamaz
    if (ctx.isEditorBuild)
    {
        if (editorCtx.mode != EngineMode::PLAY || editorCtx.isPaused)
        {
            state.cursorLocked = false;
        }
        else if (window && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            // Play modunda ESC'ye basıldığında imleci serbest bırak (UI kontrolü için)
            state.cursorLocked = false;
        }
    }

    // Pencere odağı kaybedildiyse imleç kilidini serbest bırak
    if (window && !glfwGetWindowAttrib(window, GLFW_FOCUSED))
    {
        state.cursorLocked = false;
    }

    // Standalone oyun modunda fare tıklandığında imleci anında yeniden kilitle (Focus geri kazanma)
    if (!ctx.isEditorBuild && window)
    {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        {
            state.cursorLocked = true;
        }
    }

    // Editör modunda oyun oynanmıyorsa ve sahne kamerasında sağ tık ile uçuş yapılmıyorsa
    // imlecin kazara DISABLED kalmasını engelleyen emniyet sübabı
    if (ctx.isEditorBuild && window && (editorCtx.mode != EngineMode::PLAY || editorCtx.isPaused))
    {
        const bool cameraFlying = (editorCtx.camera && editorCtx.camera->flyState.capturingMouse);
        if (!cameraFlying && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            state.cursorLocked = false;
        }
    }

    // İmleç kilit durumu değiştiyse GLFW API'sine yansıt
    if (state.cursorLocked != m_lastCursorLockState)
    {
        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            state.cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL
        );

        state.firstMouse = true;
        m_lastCursorLockState = state.cursorLocked;
    }

    // Önceki tuş durumlarını sakla
    for (int i = 0; i <= GLFW_KEY_LAST; ++i)
    {
        state.prevKeys[i] = state.keys[i];
    }

    // Tuş durumlarını sorgula
    if (state.active && window)
    {
        for (int i = 0; i <= GLFW_KEY_LAST; ++i)
        {
            state.keys[i] = (glfwGetKey(window, i) == GLFW_PRESS);
        }
    }
    else
    {
        for (int i = 0; i <= GLFW_KEY_LAST; ++i)
        {
            state.keys[i] = false;
        }
    }

    // Fare tuş durumlarını sorgula
    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b)
    {
        state.prevMouseButtons[b] = state.mouseButtons[b];
        if (state.active && window)
        {
            state.mouseButtons[b] = (glfwGetMouseButton(window, b) == GLFW_PRESS);
        }
        else
        {
            state.mouseButtons[b] = false;
        }
    }

    // Eylemleri tuş eşlemelerine göre güncelle
    for (int a = 0; a < static_cast<int>(InputAction::COUNT); ++a)
    {
        state.prevActions[a] = state.actions[a];
        state.actions[a] = false;
    }

    const auto& bindings = map.getBindings();
    for (const auto& [key, action] : bindings)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
        {
            continue;
        }

        const int a = static_cast<int>(action);
        if (state.keys[key])
        {
            state.actions[a] = true;
        }
    }

    // Fare koordinatlarını ve delta farkını hesapla
    double x = 0.0;
    double y = 0.0;
    if (window)
    {
        glfwGetCursorPos(window, &x, &y);
    }

    const auto currentPos = glm::vec2(static_cast<float>(x), static_cast<float>(y));

    if (state.firstMouse)
    {
        state.mousePos = currentPos;
        state.mouseDelta = glm::vec2(0.0f, 0.0f);
        state.gameMouseDelta = glm::vec2(0.0f, 0.0f);
        state.firstMouse = false;
    }
    else
    {
        state.mouseDelta = currentPos - state.mousePos;

        if (state.cursorLocked)
        {
            state.gameMouseDelta = state.mouseDelta;
        }
        else
        {
            state.gameMouseDelta = glm::vec2(0.0f, 0.0f);
        }

        state.mousePos = currentPos;
    }
}
