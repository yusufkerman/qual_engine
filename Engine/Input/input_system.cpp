#include "input_system.h"
#include "../Engine/engine_context.h"
#include "../Editor/editor_context.h"
#include "input_state.h"
#include "input_map.h"

#include <GLFW/glfw3.h>


static bool s_LastCursorLockState = false;

void InputSystem::update(EngineContext& ctx, EditorContext& editorCtx)
{

    ctx.inputState.gameViewportSize = editorCtx.gameViewportSize;
    ctx.inputState.gameViewportPos = editorCtx.gameViewportPos;

    GLFWwindow* window = ctx.window;
    InputState& state = ctx.inputState;
    InputMap map = ctx.inputMap;

    // Cursor lock değiştiyse GLFW'ye uygula
    if (state.cursorLocked != s_LastCursorLockState)
    {
        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            state.cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        state.firstMouse = true;
        s_LastCursorLockState = state.cursorLocked;
    }

    // Önceki tuş durumlarını sakla
    for (int i = 0; i <= GLFW_KEY_LAST; i++)
        state.prevKeys[i] = state.keys[i];

    // Tuşları oku
    if (state.active)
    {
        for (int i = 0; i <= GLFW_KEY_LAST; i++)
            state.keys[i] = glfwGetKey(window, i) == GLFW_PRESS;
    }
    else
    {
        for (int i = 0; i <= GLFW_KEY_LAST; i++)
            state.keys[i] = false;
    }

    // Action map
    const auto& bindings = map.getBindings();

    for (auto& [key, action] : bindings)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            continue;

        int a = static_cast<int>(action);

        state.prevActions[a] = state.actions[a];
        state.actions[a] = state.keys[key];
    }

    // Mouse
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (state.firstMouse)
    {
        state.mousePos = { (float)x, (float)y };
        state.mouseDelta = { 0.0f, 0.0f };
        state.firstMouse = false;
    }
    else
    {
        state.mouseDelta =
        {
            (float)x - state.mousePos.x,
            (float)y - state.mousePos.y
        };

        // game viewport'a göre ölçeklenmiş versiyon
        if (state.gameViewportSize.x > 1 && state.gameViewportSize.y > 1)
        {
            state.gameMouseDelta.x = state.mouseDelta.x;
            state.gameMouseDelta.y = state.mouseDelta.y;
        }
        else
        {
            state.gameMouseDelta = state.mouseDelta;
        }

        state.mousePos = { (float)x, (float)y };
    }
}