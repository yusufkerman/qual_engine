#pragma once

struct EngineContext;
struct EditorContext;

class runtime_layer
{
public:
    static void update(EngineContext& ctx, EditorContext& editorCtx);
};