#pragma once
#include "input_state.h"

inline bool isKeyDown(InputState& s, int key)
{
    return s.keys[key];
}

inline bool isKeyPressed(InputState& s, int key)
{
    return s.keys[key] && !s.prevKeys[key];
}

inline bool isKeyReleased(InputState& s, int key)
{
    return !s.keys[key] && s.prevKeys[key];
}