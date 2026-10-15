/**
 * @file gameplay_input.cpp
 * @brief Gameplay Input API uygulaması.
 */

#include "gameplay_input.h"
#include "gameplay_context.h"
#include "../Input/input_state.h"

namespace gameplay
{
    bool input::is_action_pressed(InputAction action)
    {
        auto* state = context::get_input_state();
        if (!state)
        {
            return false;
        }

        const int idx = static_cast<int>(action);
        if (idx < 0 || idx >= static_cast<int>(InputAction::COUNT))
        {
            return false;
        }

        return state->actions[idx];
    }

    bool input::is_action_just_pressed(InputAction action)
    {
        auto* state = context::get_input_state();
        if (!state)
        {
            return false;
        }

        const int idx = static_cast<int>(action);
        if (idx < 0 || idx >= static_cast<int>(InputAction::COUNT))
        {
            return false;
        }

        return state->actions[idx] && !state->prevActions[idx];
    }

    bool input::is_action_just_released(InputAction action)
    {
        auto* state = context::get_input_state();
        if (!state)
        {
            return false;
        }

        const int idx = static_cast<int>(action);
        if (idx < 0 || idx >= static_cast<int>(InputAction::COUNT))
        {
            return false;
        }

        return !state->actions[idx] && state->prevActions[idx];
    }

    bool input::is_key_pressed(int glfw_key)
    {
        auto* state = context::get_input_state();
        if (!state || glfw_key < 0 || glfw_key > GLFW_KEY_LAST)
        {
            return false;
        }

        return state->keys[glfw_key];
    }

    bool input::is_key_just_pressed(int glfw_key)
    {
        auto* state = context::get_input_state();
        if (!state || glfw_key < 0 || glfw_key > GLFW_KEY_LAST)
        {
            return false;
        }

        return state->keys[glfw_key] && !state->prevKeys[glfw_key];
    }

    bool input::is_key_just_released(int glfw_key)
    {
        auto* state = context::get_input_state();
        if (!state || glfw_key < 0 || glfw_key > GLFW_KEY_LAST)
        {
            return false;
        }

        return !state->keys[glfw_key] && state->prevKeys[glfw_key];
    }

    float input::get_axis(InputAction positive, InputAction negative)
    {
        float value = 0.0f;
        if (is_action_pressed(positive))
        {
            value += 1.0f;
        }
        if (is_action_pressed(negative))
        {
            value -= 1.0f;
        }
        return value;
    }

    glm::vec2 input::get_axis_2d(InputAction forward, InputAction backward, InputAction right, InputAction left)
    {
        glm::vec2 dir{ 0.0f };
        dir.x = get_axis(right, left);
        dir.y = get_axis(forward, backward);

        if (glm::length(dir) > 0.001f)
        {
            dir = glm::normalize(dir);
        }
        return dir;
    }

    glm::vec2 input::get_mouse_delta()
    {
        auto* state = context::get_input_state();
        if (!state)
        {
            return glm::vec2{ 0.0f };
        }
        return state->gameMouseDelta;
    }

    glm::vec2 input::get_mouse_position()
    {
        auto* state = context::get_input_state();
        if (!state)
        {
            return glm::vec2{ 0.0f };
        }
        return state->mousePos;
    }

    bool input::is_cursor_locked()
    {
        auto* state = context::get_input_state();
        return state ? state->cursorLocked : false;
    }

    void input::set_cursor_locked(bool locked)
    {
        auto* state = context::get_input_state();
        if (state)
        {
            state->cursorLocked = locked;
        }
    }
}
