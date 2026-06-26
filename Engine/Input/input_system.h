#pragma once


struct GLFWwindow;
struct InputState;
class InputMap;

struct EngineContext;
struct EditorContext;

class InputSystem
{
public:
    void update(EngineContext& ctx, EditorContext& editorCtx);

};