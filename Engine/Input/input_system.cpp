/**
 * @file input_system.cpp
 * @brief Girdi okuma ve eylem eşleme döngüsünün uygulaması.
 */

#include "input_system.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"
#include "input_state.h"
#include "input_map.h"
#include <GLFW/glfw3.h>

void InputSystem::update(EngineContext& ctx, EditorContext& editorCtx)
{
    (void)editorCtx;

    auto& game = ctx.vestaSys.viewport_state().game();

    ctx.inputState.gameViewportSize = game.size;
    ctx.inputState.gameViewportPos = game.position;

    GLFWwindow* window = ctx.window;
    InputState& state = ctx.inputState;
    const InputMap& map = ctx.inputMap; // Referans ile bağlandı, gereksiz kopya önlendi

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
    if (state.active)
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

    // Eylemleri tuş eşlemelerine göre güncelle
    const auto& bindings = map.getBindings();

    for (const auto& [key, action] : bindings)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
        {
            continue;
        }

        const int a = static_cast<int>(action);

        state.prevActions[a] = state.actions[a];
        state.actions[a] = state.keys[key];
    }

    // Fare koordinatlarını ve delta farkını hesapla
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);

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

        if (state.gameViewportSize.x > 1.0f && state.gameViewportSize.y > 1.0f)
        {
            state.gameMouseDelta = state.mouseDelta;
        }
        else
        {
            state.gameMouseDelta = state.mouseDelta;
        }

        state.mousePos = currentPos;
    }
}